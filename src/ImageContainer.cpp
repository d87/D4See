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
    threadStarted = false;
    frameTimeElapsed = std::chrono::duration<float>(-999.0); // Animation won't start playing until we're ready for it
}

void ImageContainer::OpenFile(std::wstring filename, ImageFormat format) {
	this->filename = wide_to_utf8(filename);
    this->format = format;
	StartThread();
}

ImageContainer::~ImageContainer() {
    //SelectObject(memDC, oldbmp);
    //DeleteObject(hbitmap);
    threadState = 4;
    decoderThread.join();
    for (auto it = frame.begin(); it < frame.end(); it++) {
        DeleteObject(it->hBitmap);
    }

    //delete DIBitmap;
    delete image;
    DeleteDC(memDC);
}

void DecodingWork(ImageContainer *self) {

	self->image = new DecodeBuffer();
	DecodeBuffer* image = self->image;

    //HDC hWndDC = GetWindowDC(NULL);
    //self->memDC = CreateCompatibleDC(hWndDC);
    //std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    
    
	image->Open(self->filename, self->format);

    self->frame.resize(image->numSubimages);

    if (image->isAnimated)
        self->isAnimated = true;

    //std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    //std::cout << "============" << std::endl;
    //std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "[�s]" << std::endl;
    //std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() << "[ns]" << std::endl;

    self->threadInitPromise.set_value(true);
    PostMessage(self->hWnd, WM_FRAMEREADY, (WPARAM)self, NULL);

    //while ( (self->threadState < 3) && (self->subimagesReady < image->numSubimages) ) {
    while (self->threadState < 3) {

        int subimage = self->subimagesReady;

        ImageFrame* pImage = &self->frame[subimage];
        //if (!pImage)
            //return false;

        // All Windows DIBs are aligned to 4-byte (DWORD) memory boundaries. This
        // means that each scan line is padded with extra bytes to ensure that the
        // next scan line starts on a 4-byte memory boundary. The 'pitch' member
        // of the Image structure contains width of each scan line (in bytes).

        pImage->width = image->xres;
        pImage->height = image->yres;
        pImage->pitch = ((image->xres * 32 + 31) & ~31) >> 3;
        // 00110000
        // 10101111
        // 10101111 & 00011111
        pImage->pPixels = NULL;
        pImage->hdc = CreateCompatibleDC(NULL);

        //if (!pImage->hdc)
            //return false;

        memset(&pImage->info, 0, sizeof(pImage->info));

        pImage->info.bmiHeader.biSize = sizeof(pImage->info.bmiHeader);
        pImage->info.bmiHeader.biBitCount = 32;
        pImage->info.bmiHeader.biWidth = pImage->width;
        pImage->info.bmiHeader.biHeight = -pImage->height;
        pImage->info.bmiHeader.biCompression = BI_RGB;
        pImage->info.bmiHeader.biPlanes = 1;

        pImage->hBitmap = CreateDIBSection(pImage->hdc, &pImage->info,
            DIB_RGB_COLORS, (void**)&pImage->pPixels, NULL, 0);

        if (!pImage->hBitmap)
        {
            //ImageDestroy(pImage);
            //return false;
        }

        //GdiFlush(); // idk what it supposed to do, but it's causing a slowdown

        self->threadState = 1;
    

        while (self->threadState < 3 && !image->IsSubimageLoaded(subimage)) {

            DecoderBatchReturns decodeInfo = image->PartialLoad(100000, false);


            //self->memDCmutex.lock();

            //Gdiplus::BitmapData bitmapData;
            unsigned int yStart = decodeInfo.startLine;
            unsigned int yEnd = decodeInfo.endLine;
            int loadedScanlines = yEnd - yStart;
            //Gdiplus::Rect rc(0, yStart, image->xres, loadedScanlines);
            //dib->LockBits(&rc, Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &bitmapData);


            unsigned char* pRawBitmapOrig = &pImage->pPixels[yStart * pImage->pitch];

            unsigned char* oiioBufferPointer = &image->pixels[yStart * image->xstride];

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
        }

        self->subimagesReady++;

        if (self->isAnimated) {
            using namespace std::chrono_literals;
            pImage->frameDelay = std::chrono::duration<float>(image->frameDelay);
            if (self->frameTimeElapsed < 0s) {
                //self->PrevSubimage(); // seeking to previous frame, which is the same first frame at this point anyway
                self->frameTimeElapsed = pImage->frameDelay; // Will start advancing animation

                //std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
                //std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "[ms]" << std::endl;
                
            }
        }

        if (image->IsFullyLoaded())
            self->threadState = 3;
        else {
            self->frame.resize(image->numSubimages);
        }

       
    }



	self->threadPromise.set_value(true);
}

void ImageContainer::StartThread() {
    decoderThread = std::thread(DecodingWork, this);
    threadStarted = true;
    threadFinished = threadPromise.get_future();
    threadInitFinished = threadInitPromise.get_future();
}

void ImageContainer::TerminateThread() {
    delete& decoderThread;
}

bool ImageContainer::IsFinished() {
    using namespace std::chrono_literals;

    if (!threadStarted)
        return false;

    auto status = threadFinished.wait_for(0ms);
    if (status == std::future_status::ready)
        return true;
}

ImageFrame* ImageContainer::GetActiveSubimage() {
    return &frame[curFrame];
}

bool ImageContainer::PrevSubimage() {
    curFrame--;

    if (curFrame < 0)
        curFrame = subimagesReady -1;

    return true;
}

bool ImageContainer::NextSubimage() {
    curFrame++;

    if (curFrame >= image->numSubimages)
        curFrame = 0;

    if (curFrame >= subimagesReady) {
        curFrame--;
        std::cout << "Waiting on frame " << curFrame << std::endl;
        return false; // Next frame isn't ready yet
    }

    return true;
}

bool ImageContainer::AdvanceAnimation(std::chrono::duration<float> elapsed) {
    using namespace std::chrono_literals;
    frameTimeElapsed += elapsed;
    //std::cout << frameTimeElapsed.count() << "   " << GetActiveSubimage()->frameDelay.count() << std::endl;
    if (frameTimeElapsed >= GetActiveSubimage()->frameDelay) {
        if (NextSubimage()) {
            //std::cout << "Next Frame GO" << std::endl;
            frameTimeElapsed -= GetActiveSubimage()->frameDelay;
            return true;
        }
    }
    return false;
}