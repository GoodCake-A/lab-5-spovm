// lab 2.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//
#define _CRT_SECURE_NO_WARNINGS
#include <unistd.h>
#include <errno.h>
#include <aio.h>


//HANDLE free_output;




off_t read_from_file(int file, volatile void* buffer, int max_length, off_t offset);
off_t write_in_file(int file, volatile void* buffer, int length, off_t offset);






off_t read_from_file(int file, volatile void* buffer, int max_length, off_t offset)
{
	struct aiocb info;
	info.aio_fildes=file;
	info.aio_offset=offset;
	info.aio_buf=buffer;
	info.aio_nbytes=max_length;
	info.aio_sigevent.sigev_notify=SIGEV_NONE;

	aio_read(&info);
	while(aio_error(&info)==EINPROGRESS);
	off_t transf_bytes=aio_return(&info);
	return transf_bytes;
}

off_t write_in_file(int file, volatile void* buffer, int length, off_t offset)
{
	struct aiocb info;
	info.aio_fildes=file;
	info.aio_offset=offset;
	info.aio_buf=buffer;
	info.aio_nbytes=length;
	info.aio_sigevent.sigev_notify=SIGEV_NONE;

	aio_write(&info);
	while(aio_error(&info)==EINPROGRESS);
	return length;
}

