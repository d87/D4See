#include "ImageContainer.h"
#include "D4See.h"
#include "util.h"

ImageContainer::ImageContainer(HWND hWnd) {
    _Init(hWnd);
}

ImageContainer::ImageContainer(HWND hWnd, const std::wstring& filename, ImageFormat format) {
    _Init(hWnd);
	OpenFile(filename, format);
}

void ImageContainer::_Init(HWND hWnd) {
    this->hWnd = hWnd;
    frameTimeElapsed = std::chrono::duration<float>(-999.0); // Animation won't start playing until we're ready for it
}

void ImageContainer::OpenFile(const std::wstring& filename, ImageFormat format) {
	this->filename = filename; //copy
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
}

void ImageContainer::Decode() {

    try {
	    image.Open(this->filename, this->format);

        this->width = image.spec.width;
        this->height = image.spec.height;
        this->internalRotation = image.spec.internalRotation;
        this->isAnimated = image.spec.isAnimated;
    }
    catch (const std::runtime_error& e) {

        this->width = 800;
        this->height = 600;
        this->thread_error = e.what();
        this->thread_state = ThreadState::Error;
        this->threadInitPromise.set_value(true);
        PostMessage(this->hWnd, WM_FRAMEERROR, (WPARAM)this, NULL);
        LOG_ERROR(e.what());
    }

    while (this->thread_state < ThreadState::Done) {

        int subimage = this->subimagesReady;

        this->bitmap_mutex.lock();

        ImageFrame  img;
        memset(&img, 0, sizeof(ImageFrame));

        // All Windows DIBs are aligned to 4-byte (DWORD) memory boundaries. This
        // means that each scan line is padded with extra bytes to ensure that the
        // next scan line starts on a 4-byte memory boundary. The 'pitch' member
        // of the Image structure contains width of each scan line (in bytes).

        img.width = image.spec.width;
        img.height = image.spec.height;
        img.pitch = ((image.spec.width * 32 + 31) & ~31) >> 3;
        img.pPixels = NULL;
        img.pBitmap = nullptr;

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

        DecoderStatus status = InProgress;
        if (image.decoder->GetDirectPassType() != NULL) {

            ID2D1BitmapRenderTarget * pFrameRT= image.decoder->GetFrameD2D1BitmapRT();

			ID2D1Bitmap* pFrameToBeSaved = nullptr;

			HRESULT hr = pFrameRT->GetBitmap(&pFrameToBeSaved);
			if (SUCCEEDED(hr))
			{
				auto bitmapSize = pFrameToBeSaved->GetPixelSize();
				D2D1_BITMAP_PROPERTIES bitmapProp;
				pFrameToBeSaved->GetDpi(&bitmapProp.dpiX, &bitmapProp.dpiY);
				bitmapProp.pixelFormat = pFrameToBeSaved->GetPixelFormat();

				hr = pFrameRT->CreateBitmap(
					bitmapSize,
					bitmapProp,
					&img.pBitmap);
		    }

			if (SUCCEEDED(hr))
			{
				// Copy the whole bitmap
				hr = img.pBitmap->CopyFromBitmap(nullptr, pFrameToBeSaved, nullptr);
			}

			SafeRelease(pFrameToBeSaved);

            image.curSubimage++;
            status = SubimageFinished;
            image.decoder->PrepareNextFrameBitmapSource();
        }
        else {

            HRESULT hr = pRenderTarget->CreateBitmap(
                bitmapSize,
                img.pPixels,
                img.pitch,
                &bitmapProperties,
                &img.pBitmap
            );

        }

        this->frame.push_back(img);
        ImageFrame* pImage = &this->frame[subimage];

        using namespace std::chrono_literals;
        pImage->frameDelay = std::chrono::duration<float>(image.decoder->GetCurrentFrameDelay());

        this->bitmap_mutex.unlock();

        long long numBytes = (long long)pImage->height * pImage->pitch;
        std::vector<BYTE> pixels(numBytes);
        memset(&pixels[0], 0, numBytes);

        pImage->pPixels = &pixels[0];

        //GdiFlush(); // idk what it supposed to do, but it's causing a slowdown

        if (this->thread_state == ThreadState::Uninitialized) {
            this->curFrame = 0; // Set to 0 from -1
            this->thread_state = ThreadState::Initialized;
            this->threadInitPromise.set_value(true);
            PostMessage(this->hWnd, WM_FRAMEREADY, (WPARAM)this, NULL);
        }

        while (this->thread_state < ThreadState::Done && status == DecoderStatus::InProgress) {//&& !image.IsSubimageLoaded(subimage)) {

            DecoderBatchReturns decodeInfo = image.PartialLoad(200000, true);
            status = decodeInfo.status;
            //LOG(status);

            //Gdiplus::BitmapData bitmapData;
            unsigned int yStart = decodeInfo.startLine;
            unsigned int yEnd = decodeInfo.endLine;
            int loadedScanlines = yEnd - yStart;
            //Gdiplus::Rect rc(0, yStart, image.xres, loadedScanlines);
            //dib->LockBits(&rc, Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &bitmapData);


            unsigned char* pRawBitmapOrig = &pImage->pPixels[(long long)yStart * pImage->pitch];
            unsigned char* pBatchStart = pRawBitmapOrig;

            unsigned char* oiioBufferPointer = &image.pixels[(long long)yStart * image.spec.rowPitch];

            switch (image.spec.numChannels) {

            case 3:
                for (int i = 0; i < image.spec.width * loadedScanlines; i++) {
                    pRawBitmapOrig[0] = oiioBufferPointer[2];
                    pRawBitmapOrig[1] = oiioBufferPointer[1];
                    pRawBitmapOrig[2] = oiioBufferPointer[0];
                    pRawBitmapOrig[3] = 0xFF;
                    oiioBufferPointer += 3;
                    pRawBitmapOrig += 4;
                }
                break;

            case 4:
                for (int i = 0; i < image.spec.width * loadedScanlines; i++) {
                    pRawBitmapOrig[0] = oiioBufferPointer[2];
                    pRawBitmapOrig[1] = oiioBufferPointer[1];
                    pRawBitmapOrig[2] = oiioBufferPointer[0];
                    pRawBitmapOrig[3] = oiioBufferPointer[3];
                    oiioBufferPointer += 4;
                    pRawBitmapOrig += 4;
                }
                break;

            case 2:
                for (int i = 0; i < image.spec.width * loadedScanlines; i++) {
                    pRawBitmapOrig[0] = oiioBufferPointer[0];
                    pRawBitmapOrig[1] = oiioBufferPointer[0];
                    pRawBitmapOrig[2] = oiioBufferPointer[0];
                    pRawBitmapOrig[3] = oiioBufferPointer[1];;
                    oiioBufferPointer += 2;
                    pRawBitmapOrig += 4;
                }
                break;

            case 1:
                for (int i = 0; i < image.spec.width * loadedScanlines; i++) {
                    pRawBitmapOrig[0] = oiioBufferPointer[0];
                    pRawBitmapOrig[1] = oiioBufferPointer[0];
                    pRawBitmapOrig[2] = oiioBufferPointer[0];
                    pRawBitmapOrig[3] = 0xFF;
                    oiioBufferPointer += 1;
                    pRawBitmapOrig += 4;
                }
                break;
            }

            this->decoderBatchId++;

            D2D1_RECT_U rect;
            rect.left = 0;
            rect.top = yStart;
            rect.right = pImage->width;
            rect.bottom = yEnd;

            
            //this->bitmap_mutex.lock();
            pImage->pBitmap->CopyFromMemory(&rect, pBatchStart, pImage->pitch);
            //this->bitmap_mutex.unlock();
        }

        //this->thread_state = ThreadState::BatchReady;

        this->counter_mutex.lock();
        this->subimagesReady++;
        this->numSubimages = this->subimagesReady;
        //if (this->subimagesReady > 1) {
        //    this->isAnimated = true;
        //}

        this->counter_mutex.unlock();

        if (this->isAnimated) {
            if (this->frameTimeElapsed < 0s) {
                //this->PrevSubimage(); // seeking to previous frame, which is the same first frame at this point anyway
                this->frameTimeElapsed = pImage->frameDelay; // Will start advancing animation

                //std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
                //std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "[ms]" << std::endl;
            }
        }
        

        if (image.IsFullyLoaded()) {
            
            this->thread_state = ThreadState::Done;
        }
        //else {
        //    this->frame.resize(image->numSubimages);
        //}

       
    }
	//this->threadPromise.set_value(true);
}

void ImageContainer::StartThread() {
    decoderThread = std::thread([this]() {
        this->Decode();
    });
    threadInitFinished = threadInitPromise.get_future();
}

void ImageContainer::TerminateThread() {
    delete& decoderThread;
}

bool ImageContainer::IsFinished() {
    return thread_state >= ThreadState::Done;
}
int ImageContainer::GetActiveFrameIndex() {
    return curFrame;
}


inline ImageFrame* ImageContainer::GetActiveSubimage() {
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
        if (thread_state != ThreadState::Done) { // when all subimages are ready is's null
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

    if (activeSubimage) {
        if (frameTimeElapsed >= activeSubimage->frameDelay) {
            frameTimeElapsed -= activeSubimage->frameDelay;
            if (NextSubimage()) {
                return true;
            }
        }
    }
    return false;
}