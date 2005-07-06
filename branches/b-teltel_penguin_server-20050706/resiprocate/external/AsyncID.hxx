#if !defined(RESIP_ASYNC_ID_HXX)
#define RESIP_ASYNC_ID_HXX

namespace resip
{

// ?dcm? -- I originally wanted a flexible class that would map to any asyncronous
// completetion token.  One way to do this is with templates in the function
// that accept Async ids, another way is to have them passed around by
// reference, but in the xTen case(for example) ids are simle ulongs, so this
// would cause a lot of work in the glue code.  Of course, this definition is
// located here, so it would be easy to change to a lightweight wrapper class to
// other implementation.  A templatized class w/ a conversion operator would
// work.
// sometimes GC and handles make life easier.  Of course the template approach
// could involve ref. counting when necessary.

//should be small enough to happily pass by value
//   class AsyncID
//   {
//   public:
//     virtual operator==(const AsyncID& other) const = 0;
//     virtual bool operator<(const AsyncID& rhs) const = 0;
//   };

typedef unsigned long AsyncID;

//error code of zero is success. Should define an enum for implementations to
//map to.
class AsyncResult
{
   public:
      AsyncResult() : mErrorCode(0) {}
      AsyncResult(long errorCode) : mErrorCode(errorCode) {}
      void setError(long errorCode) { mErrorCode = errorCode; }
      long errorCode() const { return mErrorCode; }
      bool success() const { return mErrorCode == 0 ? true : false; }
   protected:
      long mErrorCode;
};

}

#endif
