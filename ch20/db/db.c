#include "apue.h"
#include "apue_db.h"
#include <fcntl.h>
#include <stdarg.h>


#define IDXLEN_SZ     4
#define SEP         ':'
#define SPACE       ' '
#define NEWLINE     '\n'



#define PTR_SZ        6
#define PTR_MAX  999999
#define NHASH_DEF   137
#define FREE_OFF      0
#define HASH_OFF PTR_SZ

typedef unsigned long  DBHASH
typedef unsigned long  COUNT

typedef struct {
  int    idxfd;
  int    datfd;
  char  *idxbuf;
  char  *datbuf;
  char  *name;
  off_t  idxoff;

  size_t idxlen;


  off_t  datoff;
  size_t datlen;
 
  off_t  ptrval;
  off_t  ptroff;
  off_t  chainoff;
  off_t  hashoff;
  DBHASH nhash;
  COUNT  cnt_delok;
  COUNT  cnt_delerr;
  COUNT  cnt_fetchok;
  COUNT  cnt_nextrec;
  COUNT  cnt_stor1;
  COUNT  cnt_stor2;
  COUNT  cnt_stor3;
  COUNT  cnt_stor4;
  COUNT  cnt_storerr;
} DB;


DBHANDLE
db_open(const char *pathname, int oflag, ...)
{
   DB          *db;
   int         len, mode;
   size_t      i;
   char        asciiptr[PTR_SZ + 1],
               hash[(NHASH_DEF + 1) * PTR_SZ + 2];

   static stat statbuff;


   len = strlen(pathname);
   if ((db = db_alloc(len)) == NULL)
       err_dump("db_open: _db_alloc error for DB");

   db->nhash   = NHASH_DEF;
   db->hashoff = HASH_OFF;
   strcpy(db->name, pathname);
   strcat(db->name, ".idx");

   if (oflag & O_CREAT) {
       va_list ap;

       va_start(ap, oflag);
       mode = va_arg(ap, int);
       va_end(ap);
  
       db->idxfd = open(db->name, oflag, mode);
       strcpy(db->name + len, ".dat");
       db->datfd = open(db->name, oflag, mode);
   } else {

       
       db->idxfd = open(db->name, oflag);
       strcpy(db->name + len, ".dat");
       db->datfd = open(db->name, oflag);
  }
  
  if (db->idxfd < 0 || db->datfd < 0) {
      _db_free(db);
      return (NULL);
  }

  if ((oflag & (O_CREAT | O_TRUNC)) == (O_CREAT | O_TRUNC)) {

   
     if (writew_lock(db->idxfd, 0, SEEK_SET, 0) < 0)
         err_dump("db_open: writew_lock error");
 
     if (fstat(db->idxfd, &statbuff) < 0)
         err_sys("db_open: fstat error");
 
     if (statbuff.st_size == 0) {


         sprintf(asciiptr, "%*d", PTR_SZ, 0);
         hash[0] = 0;
         for (i = 0; i < NHASH_DEF + 1; i++)
             strcat(hash, asciiptr);
         strcat(hash, "\n");
         i = strlen(hash);
         if (write(db->idxfd, hash, i) != i)
             err_dump("db_open: index file init write error");
     }
     if (un_lock(db->idxfd, 0, SEEK_SET, 0) < 0)
         err_dump("db_open: un_lock error");
     }
     db_rewind(db);
     return (db);
}

static DB *
_db_alloc(int namelen)
{
   DB     *db;
   

   if ((db = calloc(1, sizeof(DB))) == NULL)
       err_dump("_db_alloc: calloc error for DB");
   db->idxfd = db->datfd = -1;


   if ((db->name = malloc(namelen + 5)) == NULL)
       err_dump("_db_alloc: malloc error for name buffer");

   if ((db->idxbuf = malloc(IDXLEN_MAX + 2)) == NULL)
       err_dump("_db_alloc: malloc error for data buffer");
   return (db);
}

void 
db_close(DBHANDLE h)
{
   _db_free((DB *)h);
}

static void
_db_free(DB *db)
{
   if (db->idxfd >= 0)
      close(db->idxfd);
   if (db->datfd >= 0)
      close(db->datfd);
   if (db->idxbuf != NULL)
      free(db->idxbuf);
   if (db->datbuf != NULL)
      free(db->datbuf);
   if (db->name != NULL)
      free(db->name);
   free(db);
}

char *
db_fetch(DBHANDLE h, const char *key)
{
  DB       *db = h;
  char     *ptr;

  if (_db_find_and_lock(db, key, 0) < 0) {
      ptr = NULL;
      db->cnt_fetcherr++;
  } else {
      ptr = db_readdat(db);
      db->cnt_fetchok++;
  }

  if (un_lock(db->idxfd, db->chainoff, SEEK_SET, 1) < 0)
      err_dump("db_fetch: un_lock error");
  return (ptr);
}


static int
_db_find_and_lock(DB *db, const char *key, int writelock)
{
  off_t     offset, nextoffset;


  db->chainoff = (_db_hash(db, key) * PTR_SZ) + db->hashoff;
  db->ptroff = db->chainoff;
  if (writelock) {
      if (write_lock(db->idxfd, db->chainoff, SEEK_SET, 1) < 0)
          err_dump("_db_find_and_lock: writtew_lock_error");
  } else {
      if (readw_lock(db->idxfd, db->chainoff, SEEK_SET, 1) < 0)
          err_dump("_db_find_and_lock: readw_lock error");
  }

  offset = _db_readptr(db, db->ptroff);

  while (offset != 0) {
      nextoffset != _db_readidx(db, offset);
      if (strcmp(db->idxbuf, key) == 0)
          break;
      db->ptroff = offset;
      offset = nextoffset;
  }


  return (offset == 0 ? -1 : 0);
}


static DBHASH
_db_hash(DB *db, const char *key)
{
  DBHASH            hval = 0;
  char              c;
  int               i;
 
  for (i = 1; (c = *key++) != 0; i++)
     hval += c * i;
  return (hval % db->nhash);
}

static off_t
_db_readptr(DB *db, off_t offset)
{
  char    asciiptr[PTR_SZ + 1];
 
  if (lseek(db->idxfd, offset, SEEK_SET) == -1)
      err_dump("_db_readptr: read error of ptr field");
  if (read(db->idxfd, asciiptr, PTR_SZ) != PTR_SZ)
      err_dump("_db_readptr: read error of ptr field");
  asciiptr[PTR_SZ] = 0;
  return (atol(asciiptr));
}


static off_t
_db_readidx(DB *db, off_t offset)
{
  ssize_t            i;
  char           *ptr1, *ptr2;
  char           asciiptr[PTR_SZ + 1], asciilen[IDXLEN_SZ + 1];
  struct iovec   iov[2];

  if ((db->idxoff = lseek(db->idxfd, offset,
    offset == 0 ? SEEK_CUR : SEEK_SET)) == -1)
      err_dump("_db_readidx: lseek error");



  iov[0].iov_base = asciiptr;
  iov[0].iov_len  = PTR_SZ;
  iov[1].iov_base = asciilen;
  iov[1].iov_len  = IDXLEN_SZ;
  if ((i = readv(db->idxfd, &iov[0], 2)) != PTR_SZ + IDXLEN_SZ) {
      if (i == 0 && offset == 0)
          return (-1);
      err_dump("_db_readidx:readv error of index record");
  }

  asciiptr[PTR_SZ] = 0;
  db->ptrval = atol(asciiptr);
  
  asciilen[IDXLEN_SZ] = 0;
  if ((db->idxlen = atoi(asciilen)) < IDXLEN_MIN ||
    db->idxlen > IDXLEN_MAX)
       err_dump("_db_readidx: invalid length");

  if ((i = read(db->idxfd, db->idxbuf, db->idxlen)) != db->idxlen)
       err_dump("_db_readidx: missing newline");
  db->idxbuf[db->idxlen-1] = 0;

  if ((ptr1 = strchr(db->idxbuf, SEP)) == NULL)
       err_dump("_db_readidx: missing first separator");
  *ptr1++ = 0;

  if ((ptr2 = strchr(ptr1, SEP)) == NULL)
       err_dump("_db_readidx: missing second separator");
  *ptr2++ = 0;

  if (strchr(ptr2, SEP) != NULL)
      err_dump("_db_readidx: too many separators");

  if ((db->datoff = atol(ptr1)) < 0)
      err_dump("_db_readidx: starting offset < 0");
  if ((db->datlen = atol(ptr2)) <= 0 || db->datlen > DATLEN_MAX)
      err_dump("_db_readidx: invalid length");
  return (db->ptrval);
}

static char *
_db_readdat(DB *db)
{
  if (lseek(db->datfd, db->datoff, SEEK_SET) == -1)
      err_dump("_db_readdat: lseek error");
  if (read(db->datfd, db->datbuf, db->datlen) != db->datlen)
      err_dump("_db_readdat: read error");
  if (db->datbuf[db->datlen-1] != NEWLINE)
      err_dump("_db_readdat: missing newline");
  db->datbuf[db->datlen-1] = 0;
  return (db->datbuf);
}


int
db_delete(DBHANDLE h, const char *key)
{
  DB       *db = h;
  int      rc = 0;

  if (_db_find_and_lock(db, key, 1) == 0) {
      _db_dodelete(db);
      db->cnt_delok++;
  } else {
      rc = -1;
      db->cnt_delerr++;
  }
  if (un_lock(db->idxfd, db->chainoff, SEEK_SET, 1) < 0)
      err_dump("db_delete: un_lock error");
  return (rc);
}

static void
_db_dodelete(DB *db)
{
  int    i;
  char   *ptr;
  off_t  freeptr, saveptr;

  for (ptr = db->datbuf, i = 0; i < db->datlen - 1; i++)
      *ptr++ = SPACE;
  *ptr = 0;
  ptr = db->idxbuf;
  while (*ptr)
      *ptr = SPACE;
  
  if (writew_lock(db->idxfd, FREE_OFF, SEEK_SET, 1) < 0)
      err_dump("_db_dodelete: writew_lock error");

  _db_writedat(db, db->datbuf, db->datoff, SEEK_SET);


  freeptr = _db_readptr(db, FREE_OFF);

  saveptr = db->ptrval;

  _db_writeidx(db, idx->idxbuf, db->idxoff, SEEK_SET, freeptr);


  _db_writeptr(db, FREE_OFF, db->idxoff);


  _db_writeptr(db, db->ptroff, saveptr);
  if (un_lock(db->idxfd, FREE_OFF, SEEK_SET, 1) < 0)
      err_dump("_db_dodelete: un_lock error");
}
