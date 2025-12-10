/* Copyright (c) 2002-2025. The SimGrid Team. All rights goterved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>

#define assert_perror(cond, ...)                                                                                       \
  if (!(cond)) {                                                                                                       \
    fprintf(stderr, __VA_ARGS__);                                                                                      \
    perror("");                                                                                                        \
    return 1;                                                                                                          \
  }

int main(void)
{
  const char* path = "/tmp/sthread_test_disk_API.tmp";
  unlink(path);

  // ----------------------------------
  fprintf(stderr, "== TEST: basic open/close ==\n");
  {
    int fd = open(path, O_CREAT | O_RDWR, 0600);
    assert_perror(fd >= 0, "open failed.");
    fprintf(stderr, "open() ok. fd=%d\n", fd);

    assert_perror(close(fd) == 0, "close failed.");
    fprintf(stderr, "close() ok\n");
  }

  // ----------------------------------
  fprintf(stderr, "\n== TEST: Unlinking files ==\n");
  {
    int res = unlink("/tmp/sthread-WeirdFile-ThatShallNotExist");
    assert_perror(res == -1, "Unlinking inexistant file does not fail. ");
    fprintf(stderr, "unlink() on a file that does not exist fails appropriately: %s\n", strerror(errno));

    res = unlink(path);
    assert_perror(res == 0, "Unlinking existing file failed: ");
    fprintf(stderr, "unlink() on a file that exist works.\n");
  }

  // ----------------------------------
  fprintf(stderr, "\n== TEST: write; close/open; read ==\n");
  {
    int fd = open(path, O_CREAT | O_RDWR, 0600);
    assert_perror(fd >= 0, "open() failed while reopening.");

    char buff[512] = "TOTO";
    ssize_t got    = write(fd, buff, strlen(buff) + 1);
    assert_perror(got = strlen(buff) + 1, "Writing the string TOTO did not write 5 chars but %zd", got);
    fprintf(stderr, "write() ok\n");

    assert_perror(close(fd) == 0, "close failed.");
    fd = open(path, O_CREAT | O_RDWR, 0600);
    assert_perror(fd >= 0, "open() failed while reopening.");

    memset(buff, 0, 512);
    got = read(fd, buff, 512);
    assert_perror(got = 5, "Reading the string TOTO did not read 5 chars but %zd.", got);
    assert_perror(strncmp(buff, "TOTO", 5) == 0, "Reading the string TOTO did not yield TOTO but '%s'.", buff);
    fprintf(stderr, "read() ok\n");
    assert_perror(close(fd) == 0, "close failed.");
  }

  // ----------------------------------
  fprintf(stderr, "\n== TEST: reading and writing to closed fd is forbidden ==\n");
  {

    int fd = open(path, O_CREAT | O_RDWR, 0600);
    assert_perror(fd >= 0, "open() failed while reopening.");
    assert_perror(close(fd) == 0, "close failed.");

    char c      = 'A';
    ssize_t got = write(fd, &c, 1);
    assert_perror(got == -1, "Writing to a closed fd did work. Return: %zd. ", got);
    fprintf(stderr, "Writing to a closed fd failed as expected: %s\n", strerror(errno));

    got = read(fd, &c, 1);
    assert_perror(got == -1, "Reading from a closed fd did work. Return: %zd. ", got);
    fprintf(stderr, "Reading from a closed fd failed as expected: %s\n", strerror(errno));
  }

  // ----------------------------------
  fprintf(stderr, "\n== TEST: respect read-only and write-only flags ==\n");
  {
    char c;
    int fd = open(path, O_RDONLY, 0);
    assert_perror(fd > 0, "reopenning");

    ssize_t got = write(fd, "x", 1);
    assert_perror(got < 0, "ERROR: wrote to O_RDONLY fd: ");
    fprintf(stderr, "Write on O_RDONLY failed as expected: %s\n", strerror(errno));
    assert_perror(close(fd) == 0, "close failed.");

    fd = open(path, O_WRONLY, 0);
    assert_perror(fd > 0, "reopenning");

    got = read(fd, &c, 1);
    assert_perror(got < 0, "ERROR: read from O_WRONLY fd.\n");
    fprintf(stderr, "Reading from O_WRONLY failed as expected: %s\n", strerror(errno));
    assert_perror(close(fd) == 0, "close failed.");
  }

  // ----------------------------------
  fprintf(stderr, "\n== TEST: fstat() and lseek() ==\n");
  {
    char c;
    int fd = open(path, O_CREAT | O_RDWR, 0644);
    assert_perror(fd >= 0, "open");

    char buff[] = "0123456789";
    ssize_t got = write(fd, buff, 10);
    assert_perror(got == 10, "Writting 10 chars returned %zd", got);

    struct stat st;
    assert_perror(fstat(fd, &st) == 0, "fstat() failed");
    assert_perror(st.st_size == 10, "fstat() size mismatch %zd instead of 10: ", st.st_size);
    fprintf(stderr, "fstat() size is correct after writing 10 chars\n");

    off_t r = lseek(fd, 4, SEEK_SET);
    assert_perror(r == 4, "lseek SET failed. Returned %zd instead of 4", r);
    got = read(fd, &c, 1);
    assert_perror(got == 1, "Reading one char did not return 1 but %zd: ", got);
    assert_perror(c == '4', "Reading the 4th char did not yield '4' but '%c': ", c);

    r = lseek(fd, -2, SEEK_CUR);
    assert_perror(r == 3, "lseek CUR failed. Returned %zd instead of 3: ", r);
    got = read(fd, &c, 1);
    assert_perror(got == 1, "Reading one char did not return 1 but %zd: ", got);
    assert_perror(c == '3', "Reading the 4th char did not yield '3' but '%c': ", c);

    r = lseek(fd, 0, SEEK_END);
    assert_perror(r == 10, "lseek END failed. Returned %zd instead of 10: ", r);
    got = read(fd, &c, 1);
    assert_perror(got == 0, "Reading beyond the end did not return 0 but %zd: ", got);

    fprintf(stderr, "lseek() whence semantics ok\n");
  }
  // ----------------------------------
  fprintf(stderr, "\n== TEST: readv()/writev() ==\n");
  {
    int fd = open(path, O_CREAT | O_RDWR, 0644);
    assert_perror(fd >= 0, "open");

    char w1[] = "ABC";
    char w2[] = "DEF";
    struct iovec iov[2];
    iov[0].iov_base = w1;
    iov[0].iov_len  = 3;
    iov[1].iov_base = w2;
    iov[1].iov_len  = 3;

    ssize_t got = writev(fd, iov, 2);
    assert_perror(got = 6, "writev(3+3) did not return 6 but %zd: ", got);
    fprintf(stderr, "writev(3+3) wrote %zd bytes\n", got);
    got = lseek(fd, 0, SEEK_SET);
    assert_perror(got == 0, "lseek(0, SET) did not return 0 but %zd", got);

    char r1[4] = {0}, r2[4] = {0};
    struct iovec iovr[2];
    iovr[0].iov_base = r1;
    iovr[0].iov_len  = 3;
    iovr[1].iov_base = r2;
    iovr[1].iov_len  = 3;
    got              = readv(fd, iovr, 2);
    assert_perror(got == 6, "readv(3+3) did not return 6 but %zd: ", got);
    fprintf(stderr, "readv(3+3) wrote %zd bytes\n", got);

    assert_perror(strcmp(r1, w1) == 0, "Vector 1 data mismatch. '%s' != '%s': ", r1, w1);
    assert_perror(strcmp(r2, w2) == 0, "Vector 2 data mismatch. '%s' != '%s': ", r2, w2);
    fprintf(stderr, "Vector data matching.\n");
  }
  // ----------------------------------
  fprintf(stderr, "\n== TEST: preadv()/pwritev() ==\n");
  {
    int fd = open(path, O_CREAT | O_RDWR, 0644);
    assert_perror(fd >= 0, "open");

    assert_perror(write(fd, "........", 8) == 8, "Preparing destination file with zeros");

    struct iovec iovw[2];
    char w1[]        = "111";
    char w2[]        = "2222";
    iovw[0].iov_base = w1;
    iovw[0].iov_len  = 3;
    iovw[1].iov_base = w2;
    iovw[1].iov_len  = 4;

    /* pwritev at offset 2 should not change file offset */
    off_t before = lseek(fd, 0, SEEK_CUR);
    ssize_t got  = pwritev(fd, iovw, 2, 2);
    assert_perror(got == 7, "pwritev(3+4) did not return 7 but %zd. ", got);
    off_t after = lseek(fd, 0, SEEK_CUR);
    assert_perror(after == before, "pwritev() changed the offset from %zd to %zd. ", before, after);
    fprintf(stderr, "pwritev(3+4) wrote %zd bytes and did not change offset.\n", got);

    /* preadv reads at offset without changing offset */
    char r1[4] = {0}, r2[5] = {0}, r3[4] = {0};
    struct iovec iovr[3];
    iovr[0].iov_base = r1;
    iovr[0].iov_len  = 2;
    iovr[1].iov_base = r2;
    iovr[1].iov_len  = 2;
    iovr[2].iov_base = r3;
    iovr[2].iov_len  = 3;
    before           = lseek(fd, 0, SEEK_CUR);
    got              = preadv(fd, iovr, 3, 2);
    after            = lseek(fd, 0, SEEK_CUR);
    assert_perror(got == 7, "preadv(2+2+3) did not return 7 but %zd", got);
    assert_perror(after == before, "preadv() changed the offset from %zd to %zd. ", before, after);
    fprintf(stderr, "preadv(2+2+3) got %zd bytes and did not change offset.\n", got);

    assert_perror(strcmp(r1, "11") == 0, "Vector 1 data mismatch. '%s' != '11': ", r1);
    assert_perror(strcmp(r2, "12") == 0, "Vector 2 data mismatch. '%s' != '12': ", r2);
    assert_perror(strcmp(r3, "222") == 0, "Vector 3 data mismatch. '%s' != '222': ", r2);
    fprintf(stderr, "preadv() returned the expected data.\n");
  }

  fprintf(stderr, "\n");
  unlink(path);
  return 0;
}
