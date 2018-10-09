/* Copyright (c) 2015-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#define BOOST_TEST_MODULE PAGESTORE
#define BOOST_TEST_DYN_LINK

bool init_unit_test(); // boost sometimes forget to give this prototype (NetBSD and other), which does not fit our paranoid flags
#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <cstring>
#include <iostream>

#include <sys/mman.h>
#include <unistd.h>

#include <memory>

#include "src/mc/sosp/PageStore.hpp"

using simgrid::mc::PageStore;

/***********************************/
// a class to hold the variable used in the test cases
class BOOST_tests {
public:
  static std::size_t pagesize;
  static std::unique_ptr<PageStore> store;
  static void* data;
  static size_t pageno[4];
  static int value;

  // member functions used by the test suite(s)
  static void Init();
  static void store_page_once();
  static void store_same_page();
  static void store_new_page();
  static void unref_pages();
  static void reallocate_page();

  static void new_content(void* data, std::size_t size);
  static void* getpage();
};

// static member datat initialization
std::size_t BOOST_tests::pagesize             = 0;
std::unique_ptr<PageStore> BOOST_tests::store = nullptr;
void* BOOST_tests::data                       = nullptr;
size_t BOOST_tests::pageno[4]                 = {0, 0, 0, 0};
int BOOST_tests::value                        = 0;

void BOOST_tests::Init()
{
  pagesize = (size_t)getpagesize();
  store    = std::unique_ptr<PageStore>(new simgrid::mc::PageStore(500));
  data     = getpage();
  BOOST_CHECK_MESSAGE(store->size() == 0, "Bad size");
}

void BOOST_tests::store_page_once()
{
  new_content(data, pagesize);
  pageno[0] = store->store_page(data);
  BOOST_CHECK_MESSAGE(store->get_ref(pageno[0]) == 1, "Bad refcount");
  const void* copy = store->get_page(pageno[0]);
  BOOST_CHECK_MESSAGE(::memcmp(data, copy, pagesize) == 0, "Page data should be the same");
  BOOST_CHECK_MESSAGE(store->size() == 1, "Bad size");
}

void BOOST_tests::store_same_page()
{
  pageno[1] = store->store_page(data);
  BOOST_CHECK_MESSAGE(pageno[0] == pageno[1], "Page should be the same");
  BOOST_CHECK_MESSAGE(store->get_ref(pageno[0]) == 2, "Bad refcount");
  BOOST_CHECK_MESSAGE(store->size() == 1, "Bad size");
}

void BOOST_tests::store_new_page()
{
  new_content(data, pagesize);
  pageno[2] = store->store_page(data);
  BOOST_CHECK_MESSAGE(pageno[0] != pageno[2], "New page should be different");
  BOOST_CHECK_MESSAGE(store->size() == 2, "Bad size");
}

void BOOST_tests::unref_pages()
{
  store->unref_page(pageno[0]);
  BOOST_CHECK_MESSAGE(store->get_ref(pageno[0]) == 1, "Bad refcount");
  BOOST_CHECK_MESSAGE(store->size() == 2, "Bad size");
  store->unref_page(pageno[1]);
  BOOST_CHECK_MESSAGE(store->size() == 1, "Bad size");
}

void BOOST_tests::reallocate_page()
{
  new_content(data, pagesize);
  pageno[3] = store->store_page(data);
  BOOST_CHECK_MESSAGE(pageno[0] == pageno[3], "Page was not reused");
  BOOST_CHECK_MESSAGE(store->get_ref(pageno[3]) == 1, "Bad refcount");
  BOOST_CHECK_MESSAGE(store->size() == 2, "Bad size");
}

void BOOST_tests::new_content(void* data, std::size_t size)
{
  value++;
  ::memset(data, value, size);
}

void* BOOST_tests::getpage()
{
  return mmap(nullptr, getpagesize(), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

namespace utf = boost::unit_test; // for test case dependence

BOOST_AUTO_TEST_SUITE(PAGESTORE)
BOOST_AUTO_TEST_CASE(Init)
{
  BOOST_tests::Init();
}

BOOST_AUTO_TEST_CASE(store_page_once, *utf::depends_on("PAGESTORE/Init"))
{
  BOOST_tests::store_page_once();
}

BOOST_AUTO_TEST_CASE(store_same_page, *utf::depends_on("PAGESTORE/store_page_once"))
{
  BOOST_tests::store_same_page();
}

BOOST_AUTO_TEST_CASE(store_new_page, *utf::depends_on("PAGESTORE/store_same_page"))
{
  BOOST_tests::store_new_page();
}

BOOST_AUTO_TEST_CASE(unref_pages, *utf::depends_on("PAGESTORE/store_new_page"))
{
  BOOST_tests::unref_pages();
}

BOOST_AUTO_TEST_CASE(reallocate_page, *utf::depends_on("PAGESTORE/unref_pages"))
{
  BOOST_tests::reallocate_page();
}
BOOST_AUTO_TEST_SUITE_END()
