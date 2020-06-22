#include "ImageContainer.h"
#include "D4See.h"
#include "util.h"

ImageContainer::ImageContainer(HWND hWnd) {
    _Init(hWnd);
}

ImageContainer::ImageContainer(HWND hWnd, std::wstring filename, ImageFormat format) {
    _Init(hWnd);
	OpenFile(filename, format);
}

void ImageContainer::_Init(HWND hWnd) {
    this->hWnd = hWnd;
    frameTimeElapsed = std::chrono::duration<float>(-999.0); // Animation won't start playing until we're ready for it
}

void ImageContainer::OpenFile(std::wstring filename, ImageFormat format) {
	this->filename = wide_to_utf8(filename);
    this->format = format;
	StartThread();
}

ImageContainer::~ImageContainer() {
    thread_state = ThreadState::Abort;
    decoderThread.join();

    //LOG("Deleting {0}", filename);
    for (auto it = frame.begin(); it < frame.end(); it++) {
        //DeleteObject(it->hBitmap);
        if (it->pBitmap)
            it->pBitmap->Release();
    }

    if (image)
        delete image;
}

void DecodingWork(ImageContainer *self) {

    
    self->image = new DecodeBuffer();
    

    DecodeBuffer* image = self->image;

    bool threadInitDone = false;

    try {
	    image->Open(self->filename, self->format);

        self->width = image->xres;
        self->height = image->yres;
        self->isAnimated = image->isAnimated;
    }
    catch (const std::runtime_error& e) {

        self->width = 800;
        self->height = 600;
        self->thread_error = e.what();
        self->thread_state = ThreadState::Error;
        self->threadInitPromise.set_value(true);
        PostMessage(self->hWnd, WM_FRAMEERROR, (WPARAM)self, NULL);
        LOG_ERROR(e.what());
    }

    while (self->thread_state < ThreadState::Done) {

        int subimage = self->subimagesReady;

        self->bitmap_mutex.lock();

        ImageFrame  img;
        memset(&img, 0, sizeof(ImageFrame));

        // All Windows DIBs are aligned to 4-byte (DWORD) memory boundaries. This
        // means that each scan line is padded with extra bytes to ensure that the
        // next scan line starts on a 4-byte memory boundary. The 'pitch' member
        // of the Image structure contains width of each scan line (in bytes).

        img.width = image->xres;
        img.height = image->yres;
        img.pitch = ((image->xres * 32 + 31) & ~31) >> 3;
        img.pPixels = NULL;

        D2D1_SIZE_U bitmapSize;
        bitmapSize.height = img.height;
        bitmapSize.width = img.width;

        D2D1_BITMAP_PROPERTIES bitmapProperties;
        bitmapProperties.dpiX = 96;
        bitmapProperties.dpiY = 96;
        bitmapProperties.pixelFormat = D2D1::PixelFormat(
            DXGI_FORMAT_B8G8R8A8_UNORM,
            D2D1_ALPHA_MODE_IGNORE
        );

        

        HRESULT hr = pRenderTarget->CreateBitmap(
            bitmapSize,
            img.pPixels,
            img.pitch,
            &bitmapProperties,
            &img.pBitmap
        );

        self->frame.push_back(img);
        ImageFrame* pImage = &self->frame[subimage];

        using namespace std::chrono_literals;
        pImage->frameDelay = std::chrono::duration<float>(image->frameDelay);

        self->bitmap_mutex.unlock();

        long long numBytes = (long long)pImage->height * pImage->pitch;
        std::vector<BYTE> pixels(numBytes);
        memset(&pixels[0], 0, numBytes);

        pImage->pPixels = &pixels[0];

        //GdiFlush(); // idk what it supposed to do, but it's causing a slowdown

        if (!threadInitDone) {
            threadInitDone = true;
            self->curFrame = 0; // Set to 0 from -1
            self->thread_state = ThreadState::Initialized;
            self->threadInitPromise.set_value(true);
            PostMessage(self->hWnd, WM_FRAMEREADY, (WPARAM)self, NULL);
        }
    

        while (self->thread_state < ThreadState::Done && !image->IsSubimageLoaded(subimage)) {

            DecoderBatchReturns decodeInfo = image->PartialLoad(200000, true);


            

            //Gdiplus::BitmapData bitmapData;
            unsigned int yStart = decodeInfo.startLine;
            unsigned int yEnd = decodeInfo.endLine;
            int loadedScanlines = yEnd - yStart;
            //Gdiplus::Rect rc(0, yStart, image->xres, loadedScanlines);
            //dib->LockBits(&rc, Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &bitmapData);


            unsigned char* pRawBitmapOrig = &pImage->pPixels[(long long)yStart * pImage->pitch];
            unsigned char* pBatchStart = pRawBitmapOrig;

            unsigned char* oiioBufferPointer = &image->pixels[(long long)yStart * image->xstride];

            switch (image->channels) {

            case 3:
                for (int i = 0; i < image->xres * loadedScanlines; i++) {
                    pRawBitmapOrig[0] = oiioBufferPointer[2];
                    pRawBitmapOrig[1] = oiioBufferPointer[1];
                    pRawBitmapOrig[2] = oiioBufferPointer[0];
                    pRawBitmapOrig[3] = 0xFF;
                    oiioBufferPointer += 3;
                    pRawBitmapOrig += 4;
                }
                break;

            case 4:
                for (int i = 0; i < image->xres * loadedScanlines; i++) {
                    pRawBitmapOrig[0] = oiioBufferPointer[2];
                    pRawBitmapOrig[1] = oiioBufferPointer[1];
                    pRawBitmapOrig[2] = oiioBufferPointer[0];
                    pRawBitmapOrig[3] = oiioBufferPointer[3];
                    oiioBufferPointer += 4;
                    pRawBitmapOrig += 4;
                }
                break;

            case 2:
                for (int i = 0; i < image->xres * loadedScanlines; i++) {
                    pRawBitmapOrig[0] = oiioBufferPointer[0];
                    pRawBitmapOrig[1] = oiioBufferPointer[0];
                    pRawBitmapOrig[2] = oiioBufferPointer[0];
                    pRawBitmapOrig[3] = oiioBufferPointer[1];;
                    oiioBufferPointer += 2;
                    pRawBitmapOrig += 4;
                }
                break;

            case 1:
                for (int i = 0; i < image->xres * loadedScanlines; i++) {
                    pRawBitmapOrig[0] = oiioBufferPointer[0];
                    pRawBitmapOrig[1] = oiioBufferPointer[0];
                    pRawBitmapOrig[2] = oiioBufferPointer[0];
                    pRawBitmapOrig[3] = 0xFF;
                    oiioBufferPointer += 1;
                    pRawBitmapOrig += 4;
                }
                break;
            }

            self->decoderBatchId++;

            D2D1_RECT_U rect;
            rect.left = 0;
            rect.top = yStart;
            rect.right = pImage->width;
            rect.bottom = yEnd;

            
            //self->bitmap_mutex.lock();
            pImage->pBitmap->CopyFromMemory(&rect, pBatchStart, pImage->pitch);
            //self->bitmap_mutex.unlock();
        }

        //self->thread_state = ThreadState::BatchReady;

        self->counter_mutex.lock();
        self->subimagesReady++;
        self->numSubimages = self->subimagesReady;
        if (self->format == ImageFormat::GIF && self->subimagesReady > 1) {
            self->isAnimated = true;
        }

        self->counter_mutex.unlock();

        if (self->isAnimated) {
            ;
            if (self->frameTimeElapsed < 0s) {
                //self->PrevSubimage(); // seeking to previous frame, which is the same first frame at this point anyway
                self->frameTimeElapsed = pImage->frameDelay; // Will start advancing animation

                //std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
                //std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "[ms]" << std::endl;
            }
        }
        

        if (image->IsFullyLoaded()) {
            
            self->thread_state = ThreadState::Done;
        }
        //else {
        //    self->frame.resize(image->numSubimages);
        //}

       
    }

    delete self->image;
    self->image= nullptr;

	//self->threadPromise.set_value(true);
}

void ImageContainer::StartThread() {
    decoderThread = std::thread(DecodingWork, this);  
    threadInitFinished = threadInitPromise.get_future();
}

void ImageContainer::TerminateThread() {
    delete& decoderThread;
}

bool ImageContainer::IsFinished() {
    return thread_state >= ThreadState::Done;
}

ImageFrame* ImageContainer::GetActiveSubimage() {
    if (curFrame == -1) return nullptr;
    return &frame[curFrame];
}

//bool ImageContainer::PrevSubimage() {
//    curFrame--;
//
//    if (curFrame < 0)
//        curFrame = subimagesReady -1;
//
//    return true;
//}

bool ImageContainer::IsSubimageReady(int si) {
    counter_mutex.lock();
    int siReady = subimagesReady;
    counter_mutex.unlock();
    if (si >= siReady) {
        if (image != nullptr) { // when all subimages are ready is's null
            return false; // Next frame isn't ready yet
        }
    }
    return true;
}

bool ImageContainer::NextSubimage() {
    // All of the thread-safety stuff is supposed to be avoided
    // by using IsSubimageReady() before attempting to switch subimage
    curFrame++;
    if (curFrame >= subimagesReady/* thread-unsafe?*/) {
        curFrame = 0;
    }

    return true;
}

bool ImageContainer::AdvanceAnimation(std::chrono::duration<float> elapsed) {
    using namespace std::chrono_literals;
    frameTimeElapsed += elapsed;

    if (!IsSubimageReady(curFrame + 1)) return false;

    auto activeSubimage = GetActiveSubimage();

    if (frameTimeElapsed >= activeSubimage->frameDelay) {
        frameTimeElapsed -= activeSubimage->frameDelay;
        if (NextSubimage()) {
            return true;
        }
    }
    return false;
}