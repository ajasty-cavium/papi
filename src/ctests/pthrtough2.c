#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include "papi_test.h"

#define NITER 2000

void *Thread(void *data)
{
  int ret, evtset;

  if ((ret=PAPI_register_thread()) != PAPI_OK)
    test_fail(__FILE__, __LINE__, "PAPI_thread_init", ret);
  
  evtset=PAPI_NULL;
  if ((ret=PAPI_create_eventset(&evtset)) != PAPI_OK)
    test_fail(__FILE__, __LINE__, "PAPI_create_eventset", ret);
  
  if ((ret=PAPI_destroy_eventset(&evtset)) != PAPI_OK)
    test_fail(__FILE__, __LINE__, "PAPI_destroy_eventset", ret);
  
  if ((ret=PAPI_unregister_thread()) != PAPI_OK)
    test_fail(__FILE__, __LINE__, "PAPI_unregister_thread", ret);
  
  return(NULL);
}

int main( int argc, char* argv[] )
{
  int j;
  pthread_t *th = NULL;
  pthread_attr_t attr;
  int ret;
  long nthr;

  ret=PAPI_library_init(PAPI_VER_CURRENT);
  if (ret != PAPI_VER_CURRENT)
    test_fail(__FILE__,__LINE__,"PAPI_library_init",ret);

  if ((ret=PAPI_thread_init(pthread_self)!=PAPI_OK))
    test_fail(__FILE__,__LINE__,"PAPI_thread_init",ret);

  pthread_attr_init(&attr);
#ifdef PTHREAD_CREATE_UNDETACHED
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_UNDETACHED);
#endif
#ifdef PTHREAD_SCOPE_SYSTEM
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
#endif
  
  nthr = NITER;
  
  printf("Creating %d threads for %d iterations each of:\n",NITER,1);
  printf("\tregister\n");
  printf("\tcreate_eventset\n");
  printf("\tdestroy_eventset\n");
  printf("\tunregister\n");

  th = (pthread_t *)malloc(nthr*sizeof(pthread_t *));
  if (th == NULL)
    test_fail(__FILE__,__LINE__,"malloc",PAPI_ESYS);

  for (j=0;j<nthr;j++)
    {
      ret = pthread_create(&th[j], &attr, &Thread, NULL);
      if (ret) 
	test_fail(__FILE__, __LINE__, "pthread_create", PAPI_ESYS);
    }

  for (j=0;j<nthr;j++)
    {
      pthread_join(th[j], NULL);
    }

  test_pass(__FILE__, NULL, 0);
  exit(1);
}