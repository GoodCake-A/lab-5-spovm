// lab 2.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <stdio.h>
#include <time.h>
#include <list>
#include <sstream>
#include <string.h>
#include <pthread.h>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <errno.h>
#include <termios.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <string>
#include <dlfcn.h>
#include <dirent.h>

using namespace std;

pthread_mutex_t reading;
pthread_mutex_t writing;
pthread_mutex_t ready_to_exchange;
pthread_mutex_t ready_to_exit;
volatile int number_of_files;
volatile char w_buffer[10000];
volatile char r_buffer[10000];
volatile int number_of_bytes;
//HANDLE free_output;

void * writer(void *info);
void * reader(void *thread_number);


off_t (*read_from_file)(int file, volatile void* buffer, int max_length, off_t offset);
off_t (*write_in_file)(int file, volatile void* buffer, int length, off_t offset);
void signal_unlock(int signo);




int main()
{
    void *lib=dlopen("./mylibrary.so",RTLD_NOW);
    if(lib==NULL)
    {
        cout<<"Library error\n";
        return 0;
    }
    read_from_file=(off_t (*)(int, volatile void*, int, off_t))dlsym(lib,"read_from_file");
    write_in_file=(off_t (*)(int, volatile void*, int, off_t))dlsym(lib,"write_in_file");
	//free_output = CreateEventA(NULL, FALSE, TRUE, "Free");
	cout<<"Enter the number of files\n";
	int input;
	cin>>input;
	number_of_files=input;
	pthread_mutex_init(&reading,NULL);
	pthread_mutex_init(&writing,NULL);
	pthread_mutex_init(&ready_to_exit,NULL);
	pthread_t r_thread, w_thread;
    //pthread_mutex_lock(&ready_to_exchange);
	pthread_mutex_lock(&ready_to_exit);
	pthread_mutex_lock(&reading);
	pthread_mutex_lock(&writing);
    pthread_create(&r_thread,NULL,reader,nullptr);
    pthread_create(&w_thread,NULL,writer,nullptr);
    cout<<"Creating new file...";
    //writer(nullptr);
    pthread_join(w_thread,NULL);
    cout<<"\nCompleted.\n";
	pthread_mutex_destroy(&reading);
	pthread_mutex_destroy(&writing);
	pthread_mutex_destroy(&ready_to_exit);
	dlclose(lib);
}

void * reader(void *info)
{
	while (number_of_files)
	{
		string f_name=to_string(number_of_files);
		f_name= f_name+".txt";
		int file=open(f_name.c_str(),O_RDONLY);
		off_t pos=0;
		do
		{
			off_t number_of_bytes_tmp=read_from_file(file, r_buffer,10000,pos);
			//pthread_mutex_lock(&writing);
			//pthread_mutex_unlock(&writing);
			pthread_mutex_lock(&writing);
			pthread_mutex_unlock(&reading);
			number_of_bytes= number_of_bytes_tmp;
			for(int i =0;i<number_of_bytes;i++)
			{
				w_buffer[i]=r_buffer[i];
			}
			pthread_mutex_lock(&writing);
			pthread_mutex_unlock(&reading);
			//pthread_mutex_unlock(&ready_to_exchange);
			pos+=number_of_bytes_tmp;
		} while (number_of_bytes>=10000);
		close(file);
		number_of_files--;
	}
	pthread_mutex_unlock(&ready_to_exit);
	pthread_exit(nullptr);
	return nullptr;
}

void * writer(void *info)
{
	off_t pos=0;
	int file=open("result.txt",O_CREAT|O_WRONLY);
	do
	{
		//pthread_mutex_lock(&ready_to_exchange);
		//pthread_mutex_lock(&writing);
		pthread_mutex_unlock(&writing);
		pthread_mutex_lock(&reading);

		pthread_mutex_unlock(&writing);
		pthread_mutex_lock(&reading);
		pos+=write_in_file(file,w_buffer,number_of_bytes,pos);
		//pthread_mutex_unlock(&writing);


	} while (pthread_mutex_trylock(&ready_to_exit)==EBUSY);
	pthread_mutex_unlock(&ready_to_exit);

	close(file);
}


