/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/* Define media player and it's arguments */
static const char * const player = "mpv";
#define player_args "-really-quiet", "-fs"

/* and the extensions you want to play */
static const char * const extensions[] = {
  ".mp4", ".mkv", ".webm", ".m4v", ".wmv", ".avi", ".mpg",
  ".mpeg", ".flv", ".sfv"
};

/* The initial storage size */
#define STORAGE_SIZE 32

/*
 * for random seed, if it can't be used the current time
 * is used.
 */
#define SYSRAND "/dev/urandom"

/*
 * Storage is like the C++ vector<string>; it has a size which
 * is the number of elements in data and a capacity which is
 * the allocated room for elements.
 * When we run out of room we realloc 2*capacity more room for
 * performance.
 */
struct storage
{
  int size, capacity;
  char **data;
};

/*
 * Initialize storage.
 */
static struct storage *
storage_new (void)
{
  struct storage *s = malloc (sizeof (struct storage));

  if (s == NULL)
    return NULL;

  s->data = malloc (sizeof (char *) * STORAGE_SIZE);
  if (s->data == NULL)
    {
      free (s);
      return NULL;
    }

  s->size = 0;
  s->capacity = STORAGE_SIZE;

  return s;
}

/*
 * Adds a string to storage and if it doesn't fit allocates
 * more memory.
 */
static int
storage_add (struct storage *s, const char *string)
{
  if (s == NULL || string == NULL)
    return -1;

  /* If we're at capacity increase it by doubling it */
  if (s->size == s->capacity)
    {
      int newcap = s->capacity << 1;
      char **tmpdata;

      if (newcap < s->capacity)
        return -1;

      tmpdata = realloc (s->data, sizeof (char *) * newcap);

      if (tmpdata == NULL)
        return -1;

      s->data = tmpdata;
      s->capacity = newcap;
    }

  /* add the string */
  s->data[s->size] = malloc (strlen (string) + 1);
  if (s->data[s->size] == NULL)
    return -1;

  strcpy (s->data[s->size], string);
  ++s->size;

  return 0;
}

/*
 * Shuffle the storage
 */
static void
storage_shuffle (struct storage *s)
{
  int i, r;
  char *tmp;

  for (i = 0; i < s->size; ++i)
    {
      do
        {
          r = rand () % s->size;
        }
      while (r == i);

      tmp = s->data[i];
      s->data[i] = s->data[r];
      s->data[r] = tmp;
    }
}

/*
 * Free all storage memory;
 * - each allocated string
 * - the memory for holding all string pointers and
 * - the storage struct itself.
 */
static void
storage_free (struct storage *s)
{
  int i;

  for (i = 0; i < s->size; ++i)
    free (s->data[i]);

  free (s->data);
  free (s);
}

/*
 * See if string str ends with pattern pat ignoring
 * case.
 */
static int
string_endswidth_ic (const char * const str, const char * const pat)
{
  int slen = strlen (str), plen = strlen (pat), si, pi;

  if (slen < plen)
    return 0;

  for (si = slen, pi = plen; si >= slen - plen; --si, --pi)
    if (tolower (str[si]) != tolower (pat[pi]))
      return 0;

  return 1;
}

/*
 * See if name ends with one of the extensions given in the
 * extensions array.
 */
static int
in_extensions (const char * const name)
{
  int i;
  const int extslen = sizeof (extensions) / sizeof (extensions[0]);

  for (i = 0; i < extslen; ++i)
    if (string_endswidth_ic (name, extensions[i]))
      return 1;

  return 0;
}


/*
 * Play a file by forking and execlp the player. Execlp searches
 * the path for the executable and replaces the current process
 * with the player.
 */
static void
playfile (const char * const file)
{
  int status;
  pid_t pid = fork ();

  switch (pid)
    {
    case -1:
      perror("can't fork");
      exit (1);
    case 0:
      printf ("Playing '%s'\n", file);
      fclose (stdout);
      fclose (stderr);
      /* keep open stdin for control */
      execlp (player, player_args, file, (char *) NULL);
    default:
      wait (&status);
    }
}

/* Maximum path length, should already be defined by system */
/* #define PATH_MAX 1024 */

/* If you can't use D_DIR from dirent, stat files instead */
/* #define USE_STAT */

/*
 * Walk a directory tree from path, recursing(1) or not(0) and
 * adding files to storage for each playable file.
 */
static int
walkdir (const char * const path, int recurse, struct storage *storage)
{
  struct dirent *de;
#ifdef USE_STAT
  struct stat st;
#endif
  char fullname[PATH_MAX];
  DIR *dir = opendir (path);

  if (dir == NULL)
    return -1;

  while ((de = readdir (dir)) != NULL)
    {
      if (strcmp (de->d_name, ".") == 0
          || strcmp (de->d_name, "..") == 0)
        continue;

      snprintf (fullname, PATH_MAX, "%s/%s", path, de->d_name);

#ifdef USE_STAT
      if (lstat (fullname, &st) == -1)
        continue;

      if (recurse && S_ISDIR (st.st_mode))
        walkdir (fullname, recurse, storage);
#else
      if (recurse && de->d_type == DT_DIR)
        walkdir (fullname, recurse, storage);
#endif

      if (in_extensions (fullname))
        if (storage_add (storage, fullname) != 0)
          return -1;
    }

  closedir (dir);
  return 1;
}

int
main (int argc, char **argv)
{
  struct storage *storage = storage_new ();
  FILE *sysrand = fopen (SYSRAND, "rb");
  int i;

  if (storage == NULL)
    {
      fprintf (stderr, "can't allocate storage memory\n");
      return 1;
    }

  if (sysrand)
    {
      unsigned int rval;
      if (fread (&rval, sizeof (rval), 1, sysrand) == 1)
        srand (rval);
      else
        srand (time (NULL));
      fclose (sysrand);
    }
  else
    srand (time (NULL));

  if (argc < 2)
    walkdir (".", 1, storage);
  else
    for (i = 1; i < argc; ++i)
      walkdir (argv[i], 1, storage);

  storage_shuffle (storage);
  for (i = 0; i < storage->size; ++i)
    playfile (storage->data[i]);

  storage_free (storage);
  return 0;
}
