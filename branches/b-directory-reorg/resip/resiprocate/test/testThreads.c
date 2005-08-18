#include <stdio.h>
#include <pthread.h>

pthread_mutex_t rmutex;

void *pfunc(void *arg) {
  int lcount=(int)arg;
  pthread_mutex_lock(&rmutex);
  printf("Loop [%d]\n",lcount--);
  if (lcount)
    pfunc((void*)lcount);
  pthread_mutex_unlock(&rmutex);
}

int main(int argc) {
  pthread_t thread;
  pthread_mutexattr_t mutexattr;
  pthread_mutexattr_init(&mutexattr);
  if (argc < 2)
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&rmutex, &mutexattr);
  alarm(1);
  pthread_create(&thread, NULL, pfunc, (void*)10);
  pthread_join(thread, NULL);

}
