#ifndef SIMGRID_MC_CHUNKED_DATA_HPP
#define SIMGRID_MC_CHUNKED_DATA_HPP

#include <cstddef>
#include <cstdint>

#include <vector>

#include "src/mc/mc_forward.hpp"
#include "src/mc/AddressSpace.hpp"
#include "src/mc/PageStore.hpp"

namespace simgrid {
namespace mc {

class ChunkedData {
  PageStore* store_;
  std::vector<std::size_t> pagenos_;
public:
  ChunkedData() : store_(nullptr) {}
  ChunkedData(ChunkedData const& that)
  {
    store_ = that.store_;
    pagenos_ = that.pagenos_;
    for (std::size_t pageno : pagenos_)
      store_->ref_page(pageno);
  }
  void clear()
  {
    for (std::size_t pageno : pagenos_)
      store_->unref_page(pageno);
    pagenos_.clear();
  }
  ~ChunkedData()
  {
    clear();
  }

  ChunkedData(ChunkedData&& that)
  {
    store_ = that.store_;
    that.store_ = nullptr;
    pagenos_ = std::move(that.pagenos_);
    that.pagenos_.clear();
  }
  ChunkedData& operator=(ChunkedData const& that)
  {
    this->clear();
    store_ = that.store_;
    pagenos_ = that.pagenos_;
    for (std::size_t pageno : pagenos_)
      store_->ref_page(pageno);
    return *this;
  }
  ChunkedData& operator=(ChunkedData && that)
  {
    this->clear();
    store_ = that.store_;
    that.store_ = nullptr;
    pagenos_ = std::move(that.pagenos_);
    that.pagenos_.clear();
    return *this;
  }

  std::size_t page_count()          const { return pagenos_.size(); }
  std::size_t pageno(std::size_t i) const { return pagenos_[i]; }
  const std::size_t* pagenos()      const { return pagenos_.data(); }
  std::size_t*       pagenos()            { return pagenos_.data(); }

  const void* page(std::size_t i) const
  {
    return store_->get_page(pagenos_[i]);
  }

  ChunkedData(PageStore& store, AddressSpace& as,
    remote_ptr<void> addr, std::size_t page_count,
    const std::size_t* ref_page_numbers, const std::uint64_t* pagemap);
};

}
}

#endif
