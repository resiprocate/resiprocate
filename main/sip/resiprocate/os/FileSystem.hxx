#ifndef RESIP_FileSystem_hxx
#define RESIP_FileSystem_hxx

#include "resiprocate/os/Data.hxx"

#if !defined(WIN32)
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <dirent.h>
#endif


namespace resip
{
class FileSystem
{
   public:
      class Directory
      {
         public:
            typedef Data value_type;
            typedef value_type* pointer;
            typedef const value_type* const_pointer;
            typedef value_type& reference;
            typedef const value_type& const_reference;

            Directory(const Data& path);
            
            class iterator
            {
               public:
                  iterator(const Directory& dir);
                  ~iterator();

                  iterator& operator++();
                  bool operator!=(const iterator& rhs) const;
                  bool operator==(const iterator& rhs) const;
                  const Data& operator*() const;
                  const Data* operator->() const;
               private:
#ifdef WIN32
                  WIN32_FIND_DATA mFileData;
                  HANDLE mWinSearch;
#else
                  struct dirent* mDirent;
                  DIR* mNixDir;
#endif
                  Data mFile;
            };

            iterator begin() const;
            iterator end() const;
            const Data& getPath() const { return mPath; }
         private:
            Data mPath;
      };
};   

}

#endif
