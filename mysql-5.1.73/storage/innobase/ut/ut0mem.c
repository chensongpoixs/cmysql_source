/************************************************************************
Memory primitives

(c) 1994, 1995 Innobase Oy

Created 5/11/1994 Heikki Tuuri
*************************************************************************/

#include "ut0mem.h"

#ifdef UNIV_NONINL
#include "ut0mem.ic"
#endif

#include "mem0mem.h"
#include "os0sync.h"
#include "os0thread.h"

/* This struct is placed first in every allocated memory block */
typedef struct ut_mem_block_struct ut_mem_block_t;

/* The total amount of memory currently allocated from the OS with malloc */
ulint	ut_total_allocated_memory	= 0;

struct ut_mem_block_struct{
	UT_LIST_NODE_T(ut_mem_block_t) mem_block_list;
			/* mem block list node */
	ulint	size;	/* size of allocated memory */
	ulint	magic_n;
};

#define UT_MEM_MAGIC_N	1601650166

/* List of all memory blocks allocated from the operating system
with malloc */
UT_LIST_BASE_NODE_T(ut_mem_block_t)   ut_mem_block_list;

os_fast_mutex_t ut_list_mutex;	/* this protects the list */

ibool  ut_mem_block_list_inited = FALSE;

ulint*	ut_mem_null_ptr	= NULL;

/**************************************************************************
Initializes the mem block list at database startup. */
static void ut_mem_block_list_init(void)
/*========================*/
{
	os_fast_mutex_init(&ut_list_mutex);
	UT_LIST_INIT(ut_mem_block_list);
	ut_mem_block_list_inited = TRUE;
}

/**************************************************************************
Allocates memory. */

void* ut_malloc_low(
/*==========*/
				/* out, own: allocated memory */
	ulint	n,		/* in: number of bytes to allocate */
	ibool	assert_on_error)/* in: if TRUE, we crash mysqld if the
				memory cannot be allocated */
{
	ulint	retry_count	= 0;
	void*	ret;

	ut_ad((sizeof(ut_mem_block_t) % 8) == 0); /* check alignment ok */

	if (!ut_mem_block_list_inited) 
	{
		ut_mem_block_list_init();
	}
retry:
	os_fast_mutex_lock(&ut_list_mutex);

	ret = malloc(n + sizeof(ut_mem_block_t));

	if (ret == NULL && retry_count < 60) 
	{
		if (retry_count == 0) 
		{
			ut_print_timestamp(stderr);

			fprintf(stderr,
				"  InnoDB: Error: cannot allocate"
				" %lu bytes of\n"
				"InnoDB: memory with malloc!"
				" Total allocated memory\n"
				"InnoDB: by InnoDB %lu bytes."
				" Operating system errno: %lu\n"
				"InnoDB: Check if you should"
				" increase the swap file or\n"
				"InnoDB: ulimits of your operating system.\n"
				"InnoDB: On FreeBSD check you"
				" have compiled the OS with\n"
				"InnoDB: a big enough maximum process size.\n"
				"InnoDB: Note that in most 32-bit"
				" computers the process\n"
				"InnoDB: memory space is limited"
				" to 2 GB or 4 GB.\n"
				"InnoDB: We keep retrying"
				" the allocation for 60 seconds...\n",
				(ulong) n, (ulong) ut_total_allocated_memory,
#ifdef __WIN__
				(ulong) GetLastError()
#else
				(ulong) errno
#endif
				);
		}

		os_fast_mutex_unlock(&ut_list_mutex);

		/* Sleep for a second and retry the allocation; maybe this is
		just a temporary shortage of memory */

		os_thread_sleep(1000000);

		retry_count++;

		goto retry;
	}

	if (ret == NULL) 
	{
		/* Flush stderr to make more probable that the error
		message gets in the error file before we generate a seg
		fault */

		fflush(stderr);

		os_fast_mutex_unlock(&ut_list_mutex);

		/* Make an intentional seg fault so that we get a stack
		trace */
		/* Intentional segfault on NetWare causes an abend. Avoid this
		by graceful exit handling in ut_a(). */
#if (!defined __NETWARE__)
		if (assert_on_error)
		{
			ut_print_timestamp(stderr);

			fprintf(stderr,
				"  InnoDB: We now intentionally"
				" generate a seg fault so that\n"
				"InnoDB: on Linux we get a stack trace.\n");

			if (*ut_mem_null_ptr) ut_mem_null_ptr = 0;
		}
		else 
		{
			return(NULL);
		}
#else
		ut_a(0);
#endif
	}

	UNIV_MEM_ALLOC(ret, n + sizeof(ut_mem_block_t));

	((ut_mem_block_t*)ret)->size = n + sizeof(ut_mem_block_t);
	((ut_mem_block_t*)ret)->magic_n = UT_MEM_MAGIC_N;

	ut_total_allocated_memory += n + sizeof(ut_mem_block_t);

	UT_LIST_ADD_FIRST(mem_block_list, ut_mem_block_list,
			  ((ut_mem_block_t*)ret));
	os_fast_mutex_unlock(&ut_list_mutex);

	return((void*)((byte*)ret + sizeof(ut_mem_block_t)));
}

/**************************************************************************
Frees a memory block allocated with ut_malloc. */

void
ut_free(
/*====*/
	void* ptr)  /* in, own: memory block */
{
	ut_mem_block_t* block;

	block = (ut_mem_block_t*)((byte*)ptr - sizeof(ut_mem_block_t));

	os_fast_mutex_lock(&ut_list_mutex);

	ut_a(block->magic_n == UT_MEM_MAGIC_N);
	ut_a(ut_total_allocated_memory >= block->size);

	ut_total_allocated_memory -= block->size;

	UT_LIST_REMOVE(mem_block_list, ut_mem_block_list, block);
	free(block);

	os_fast_mutex_unlock(&ut_list_mutex);
}

/**************************************************************************
Implements realloc. This is needed by /pars/lexyy.c. Otherwise, you should not
use this function because the allocation functions in mem0mem.h are the
recommended ones in InnoDB.

man realloc in Linux, 2004:

       realloc()  changes the size of the memory block pointed to
       by ptr to size bytes.  The contents will be  unchanged  to
       the minimum of the old and new sizes; newly allocated mem?       ory will be uninitialized.  If ptr is NULL,  the	 call  is
       equivalent  to malloc(size); if size is equal to zero, the
       call is equivalent to free(ptr).	 Unless ptr is	NULL,  it
       must  have  been	 returned by an earlier call to malloc(),
       calloc() or realloc().

RETURN VALUE
       realloc() returns a pointer to the newly allocated memory,
       which is suitably aligned for any kind of variable and may
       be different from ptr, or NULL if the  request  fails.  If
       size  was equal to 0, either NULL or a pointer suitable to
       be passed to free() is returned.	 If realloc()  fails  the
       original	 block	is  left  untouched  - it is not freed or
       moved. */

void*
ut_realloc(
/*=======*/
			/* out, own: pointer to new mem block or NULL */
	void*	ptr,	/* in: pointer to old block or NULL */
	ulint	size)	/* in: desired size */
{
	ut_mem_block_t* block;
	ulint		old_size;
	ulint		min_size;
	void*		new_ptr;

	if (ptr == NULL) {

		return(ut_malloc(size));
	}

	if (size == 0) {
		ut_free(ptr);

		return(NULL);
	}

	block = (ut_mem_block_t*)((byte*)ptr - sizeof(ut_mem_block_t));

	ut_a(block->magic_n == UT_MEM_MAGIC_N);

	old_size = block->size - sizeof(ut_mem_block_t);

	if (size < old_size) {
		min_size = size;
	} else {
		min_size = old_size;
	}

	new_ptr = ut_malloc(size);

	if (new_ptr == NULL) {

		return(NULL);
	}

	/* Copy the old data from ptr */
	ut_memcpy(new_ptr, ptr, min_size);

	ut_free(ptr);

	return(new_ptr);
}

/**************************************************************************
Frees in shutdown all allocated memory not freed yet. */

void
ut_free_all_mem(void)
/*=================*/
{
	ut_mem_block_t* block;

	os_fast_mutex_free(&ut_list_mutex);

	while ((block = UT_LIST_GET_FIRST(ut_mem_block_list))) {

		ut_a(block->magic_n == UT_MEM_MAGIC_N);
		ut_a(ut_total_allocated_memory >= block->size);

		ut_total_allocated_memory -= block->size;

		UT_LIST_REMOVE(mem_block_list, ut_mem_block_list, block);
		free(block);
	}

	if (ut_total_allocated_memory != 0) {
		fprintf(stderr,
			"InnoDB: Warning: after shutdown"
			" total allocated memory is %lu\n",
			(ulong) ut_total_allocated_memory);
	}
}

/**************************************************************************
Copies up to size - 1 characters from the NUL-terminated string src to
dst, NUL-terminating the result. Returns strlen(src), so truncation
occurred if the return value >= size. */

ulint
ut_strlcpy(
/*=======*/
				/* out: strlen(src) */
	char*		dst,	/* in: destination buffer */
	const char*	src,	/* in: source buffer */
	ulint		size)	/* in: size of destination buffer */
{
	ulint	src_size = strlen(src);

	if (size != 0) {
		ulint	n = ut_min(src_size, size - 1);

		memcpy(dst, src, n);
		dst[n] = '\0';
	}

	return(src_size);
}

/**************************************************************************
Like ut_strlcpy, but if src doesn't fit in dst completely, copies the last
(size - 1) bytes of src, not the first. */

ulint
ut_strlcpy_rev(
/*===========*/
				/* out: strlen(src) */
	char*		dst,	/* in: destination buffer */
	const char*	src,	/* in: source buffer */
	ulint		size)	/* in: size of destination buffer */
{
	ulint	src_size = strlen(src);

	if (size != 0) {
		ulint	n = ut_min(src_size, size - 1);

		memcpy(dst, src + src_size - n, n + 1);
	}

	return(src_size);
}

/**************************************************************************
Return the number of times s2 occurs in s1. Overlapping instances of s2
are only counted once. */

ulint
ut_strcount(
/*========*/
				/* out: the number of times s2 occurs in s1 */
	const char*	s1,	/* in: string to search in */
	const char*	s2)	/* in: string to search for */
{
	ulint	count = 0;
	ulint	len = strlen(s2);

	if (len == 0) {

		return(0);
	}

	for (;;) {
		s1 = strstr(s1, s2);

		if (!s1) {

			break;
		}

		count++;
		s1 += len;
	}

	return(count);
}

/**************************************************************************
Replace every occurrence of s1 in str with s2. Overlapping instances of s1
are only replaced once. */

char *
ut_strreplace(
/*==========*/
				/* out, own: modified string, must be
				freed with mem_free() */
	const char*	str,	/* in: string to operate on */
	const char*	s1,	/* in: string to replace */
	const char*	s2)	/* in: string to replace s1 with */
{
	char*		new_str;
	char*		ptr;
	const char*	str_end;
	ulint		str_len = strlen(str);
	ulint		s1_len = strlen(s1);
	ulint		s2_len = strlen(s2);
	ulint		count = 0;
	int		len_delta = (int)s2_len - (int)s1_len;

	str_end = str + str_len;

	if (len_delta <= 0) {
		len_delta = 0;
	} else {
		count = ut_strcount(str, s1);
	}

	new_str = mem_alloc(str_len + count * len_delta + 1);
	ptr = new_str;

	while (str) {
		const char*	next = strstr(str, s1);

		if (!next) {
			next = str_end;
		}

		memcpy(ptr, str, next - str);
		ptr += next - str;

		if (next == str_end) {

			break;
		}

		memcpy(ptr, s2, s2_len);
		ptr += s2_len;

		str = next + s1_len;
	}

	*ptr = '\0';

	return(new_str);
}
