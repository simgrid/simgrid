/* Copyright (c) 2014-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#define BOOST_TEST_MODULE PAGESTORE
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

// /*******************************/
// /* GENERATED FILE, DO NOT EDIT */
// /*******************************/
// 
// #include <stdio.h>
// #include "xbt.h"
// /*******************************/
// /* GENERATED FILE, DO NOT EDIT */
// /*******************************/
// 
// #line 191 "mc/PageStore.cpp" 

#include <iostream>
#include <cstring>
#include <cstdint>

#include <unistd.h>
#include <sys/mman.h>

#include <memory>

#include "src/mc/PageStore.hpp"

static int value = 0;

static void new_content(void* data, std::size_t size)
{
  ::memset(data, ++value, size);
}

static void* getpage()
{
  return mmap(nullptr, getpagesize(), PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
}

BOOST_AUTO_TEST_CASE(pageStore) {

  using simgrid::mc::PageStore;

  std::cout << "Test adding/removing pages in the store" << std::endl;

  std::size_t pagesize = (size_t) getpagesize();
  std::unique_ptr<PageStore> store = std::unique_ptr<PageStore>(new simgrid::mc::PageStore(500));
  void* data = getpage();
  BOOST_CHECK_MESSAGE(store->size()==0, "Bad size");

  new_content(data, pagesize);
  size_t pageno1 = store->store_page(data);
  BOOST_CHECK_MESSAGE(store->get_ref(pageno1)==1, "Bad refcount");
  const void* copy = store->get_page(pageno1);
  BOOST_CHECK_MESSAGE(::memcmp(data, copy, pagesize)==0, "Page data should be the same");
  BOOST_CHECK_MESSAGE(store->size()==1, "Bad size");

  size_t pageno2 = store->store_page(data);
  BOOST_CHECK_MESSAGE(pageno1==pageno2, "Page should be the same");
  BOOST_CHECK_MESSAGE(store->get_ref(pageno1)==2, "Bad refcount");
  BOOST_CHECK_MESSAGE(store->size()==1, "Bad size");

  new_content(data, pagesize);
  size_t pageno3 = store->store_page(data);
  BOOST_CHECK_MESSAGE(pageno1 != pageno3, "New page should be different");
  BOOST_CHECK_MESSAGE(store->size()==2, "Bad size");

  store->unref_page(pageno1);
  BOOST_CHECK_MESSAGE(store->get_ref(pageno1)==1, "Bad refcount");
  BOOST_CHECK_MESSAGE(store->size()==2, "Bad size");
  store->unref_page(pageno2);
  BOOST_CHECK_MESSAGE(store->size()==1, "Bad size");

  new_content(data, pagesize);
  size_t pageno4 = store->store_page(data);
  BOOST_CHECK_MESSAGE(pageno1 == pageno4, "Page was not reused");
  BOOST_CHECK_MESSAGE(store->get_ref(pageno4)==1, "Bad refcount");
  BOOST_CHECK_MESSAGE(store->size()==2, "Bad size");
}
