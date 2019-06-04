// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PDF_DOCUMENT_LOADER_IMPL_H_
#define PDF_DOCUMENT_LOADER_IMPL_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "pdf/chunk_stream.h"
#include "pdf/document_loader.h"
#include "ppapi/utility/completion_callback_factory.h"

namespace chrome_pdf {

class DocumentLoaderImpl : public DocumentLoader {
 public:
  // Number was chosen in https://crbug.com/78264#c8
  static constexpr uint32_t kDefaultRequestSize = 65536;

  explicit DocumentLoaderImpl(Client* client);
  ~DocumentLoaderImpl() override;

  // DocumentLoader:
  bool Init(std::unique_ptr<URLLoaderWrapper> loader,
            const std::string& url) override;
  bool GetBlock(uint32_t position, uint32_t size, void* buf) const override;
  bool IsDataAvailable(uint32_t position, uint32_t size) const override;
  void RequestData(uint32_t position, uint32_t size) override;
  bool IsDocumentComplete() const override;
  void SetDocumentSize(uint32_t size) override;
  uint32_t GetDocumentSize() const override;
  uint32_t BytesReceived() const override;
  void ClearPendingRequests() override;

  // Exposed for unit tests.
  void SetPartialLoadingEnabled(bool enabled);
  bool is_partial_loader_active() const { return is_partial_loader_active_; }

 private:
  using DataStream = ChunkStream<kDefaultRequestSize>;
  struct Chunk {
    Chunk();
    ~Chunk();

    void Clear();

    uint32_t chunk_index = 0;
    uint32_t data_size = 0;
    std::unique_ptr<DataStream::ChunkData> chunk_data;
  };

  // Called by the completion callback of the document's URLLoader.
  void DidOpenPartial(int32_t result);

  // Call to read data from the document's URLLoader.
  void ReadMore();

  // Called by the completion callback of the document's URLLoader.
  void DidRead(int32_t result);

  bool ShouldCancelLoading() const;
  void ContinueDownload();

  // Called when we complete server request.
  void ReadComplete();

  bool SaveBuffer(char* input, uint32_t input_size);
  void SaveChunkData();

  uint32_t EndOfCurrentChunk() const;

  Client* const client_;
  std::string url_;
  std::unique_ptr<URLLoaderWrapper> loader_;

  pp::CompletionCallbackFactory<DocumentLoaderImpl> loader_factory_;

  DataStream chunk_stream_;
  bool partial_loading_enabled_ = true;
  bool is_partial_loader_active_ = false;

  static constexpr uint32_t kReadBufferSize = 256 * 1024;
  char buffer_[kReadBufferSize];

  // The current chunk DocumentLoader is working with.
  Chunk chunk_;

  // In units of Chunks.
  RangeSet pending_requests_;

  uint32_t bytes_received_ = 0;

  DISALLOW_COPY_AND_ASSIGN(DocumentLoaderImpl);
};

}  // namespace chrome_pdf

#endif  // PDF_DOCUMENT_LOADER_IMPL_H_
