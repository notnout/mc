
/**
 * \file
 * \brief Header: Utilities for VFS modules
 * \author Miguel de Icaza
 * \date 1995, 1996
 */

#ifndef MC_VFS_UTILVFS_H
#define MC_VFS_UTILVFS_H

#include <config.h>             /* HAVE_UTIMENSAT, HAVE_UTIME_H */

#if !defined (HAVE_UTIMENSAT) && defined (HAVE_UTIME_H)
#include <utime.h>
#endif

#include <sys/stat.h>

#include "lib/global.h"
#include "path.h"

/*** typedefs(not structures) and defined constants **********************************************/

/*** enums ***************************************************************************************/

/** Bit flags for vfs_url_split()
 *
 *  Modify parsing parameters according to flag meaning.
 *  @see vfs_url_split()
 */
typedef enum
{
    URL_FLAGS_NONE = 0,
    URL_USE_ANONYMOUS = 1, /**< if set, empty *user will contain NULL instead of current */
    URL_NOSLASH = 2        /**< if set, 'proto://' part in url is not searched */
} vfs_url_flags_t;

/*** structures declarations (and typedefs of structures)*****************************************/

/*** global variables defined in .c file *********************************************************/

/*** declarations of public functions ************************************************************/

int vfs_finduid (const char *name);
int vfs_findgid (const char *name);

vfs_path_element_t *vfs_url_split (const char *path, int default_port, vfs_url_flags_t flags);
int vfs_split_text (char *p);

int vfs_mkstemps (vfs_path_t ** pname_vpath, const char *prefix, const char *basename);
void vfs_die (const char *msg);
char *vfs_get_password (const char *msg);

char *vfs_get_local_username (void);

gboolean vfs_parse_filetype (const char *s, size_t *ret_skipped, mode_t * ret_type);
gboolean vfs_parse_fileperms (const char *s, size_t *ret_skipped, mode_t * ret_perms);
gboolean vfs_parse_filemode (const char *s, size_t *ret_skipped, mode_t * ret_mode);
gboolean vfs_parse_raw_filemode (const char *s, size_t *ret_skipped, mode_t * ret_mode);

void vfs_parse_ls_lga_init (void);
gboolean vfs_parse_ls_lga (const char *p, struct stat *s, char **filename, char **linkname,
                           size_t *filename_pos);
size_t vfs_parse_ls_lga_get_final_spaces (void);
gboolean vfs_parse_month (const char *str, struct tm *tim);
int vfs_parse_filedate (int idx, time_t * t);

/*** inline functions ****************************************************************************/

static inline int
vfs_utime (const char *path, mc_timesbuf_t *times)
{
#ifdef HAVE_UTIMENSAT
    return utimensat (AT_FDCWD, path, *times, AT_SYMLINK_NOFOLLOW);
#else
    return utime (path, times);
#endif
}

static inline void
vfs_get_timespecs_from_timesbuf (mc_timesbuf_t *times, mc_timespec_t *atime, mc_timespec_t *mtime)
{
#ifdef HAVE_UTIMENSAT
    atime->tv_sec = (*times)[0].tv_sec;
    atime->tv_nsec = (*times)[0].tv_nsec;
    mtime->tv_sec = (*times)[1].tv_sec;
    mtime->tv_nsec = (*times)[1].tv_nsec;
#else
    atime->tv_sec = times->actime;
    atime->tv_nsec = 0;
    mtime->tv_sec = times->modtime;
    mtime->tv_nsec = 0;
#endif
}

static inline void
vfs_get_timesbuf_from_stat (const struct stat *s, mc_timesbuf_t *times)
{
#ifdef HAVE_UTIMENSAT
#ifdef HAVE_STRUCT_STAT_ST_MTIM
    /* POSIX IEEE Std 1003.1-2008 should be the preferred way
     *
     * AIX has internal type st_timespec_t conflicting with timespec, so assign per field, for details see:
     * https://github.com/libuv/libuv/pull/4404
     */
    (*times)[0].tv_sec = s->st_atim.tv_sec;
    (*times)[0].tv_nsec = s->st_atim.tv_nsec;
    (*times)[1].tv_sec = s->st_mtim.tv_sec;
    (*times)[1].tv_nsec = s->st_mtim.tv_nsec;
#elif HAVE_STRUCT_STAT_ST_MTIMESPEC
    /* Modern BSD solution */
    (*times)[0] = s->st_atimespec;
    (*times)[1] = s->st_mtimespec;
#elif HAVE_STRUCT_STAT_ST_MTIMENSEC
    /* Legacy BSD solution */
    (*times)[0].tv_sec = s->st_atime;
    (*times)[0].tv_nsec = s->st_atimensec;
    (*times)[1].tv_sec = s->st_mtime;
    (*times)[1].tv_nsec = s->st_mtimensec;
#else
#error "Found utimensat for nanosecond timestamps, but unsupported struct stat format!"
#endif
#else
    times->actime = s->st_atime;
    times->modtime = s->st_mtime;
#endif
}

static inline void
vfs_copy_stat_times (const struct stat *src, struct stat *dst)
{
    dst->st_atime = src->st_atime;
    dst->st_mtime = src->st_mtime;
    dst->st_ctime = src->st_ctime;

#ifdef HAVE_STRUCT_STAT_ST_MTIM
    dst->st_atim.tv_nsec = src->st_atim.tv_nsec;
    dst->st_mtim.tv_nsec = src->st_mtim.tv_nsec;
    dst->st_ctim.tv_nsec = src->st_ctim.tv_nsec;
#elif HAVE_STRUCT_STAT_ST_MTIMESPEC
    dst->st_atimespec.tv_nsec = src->st_atimespec.tv_nsec;
    dst->st_mtimespec.tv_nsec = src->st_mtimespec.tv_nsec;
    dst->st_ctimespec.tv_nsec = src->st_ctimespec.tv_nsec;
#elif HAVE_STRUCT_STAT_ST_MTIMENSEC
    dst->st_atimensec = src->st_atimensec;
    dst->st_mtimensec = src->st_atimensec;
    dst->st_ctimensec = src->st_atimensec;
#endif
}

static inline void
vfs_zero_stat_times (struct stat *s)
{
    const struct stat empty = { 0 };
    vfs_copy_stat_times (&empty, s);
}

#endif
