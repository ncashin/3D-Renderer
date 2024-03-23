#pragma once

#include "buffer.h"

#include "render_manager.h"

namespace ngfx{
namespace StagingManager{
void Initialize();
void Terminate();

char* UploadToBuffer(size_t upload_size, size_t offset, Buffer* buffer);
char* UploadToImage();

void SubmitUpload(SubmitInfo submit_info);
void AwaitUploadCompletion();

extern char* mapped_pointer;
extern Buffer staging_buffer;
}
}
