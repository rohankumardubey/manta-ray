#include "../include/complex_map_2d.h"

#include "../include/standard_allocator.h"
#include "../include/image_byte_buffer.h"
#include "../include/signal_processing.h"
#include "../include/scalar_map_2d.h"
#include "../include/image_plane.h"
#include "../include/vector_map_2d.h"

#include <thread>
#include <assert.h>

manta::ComplexMap2D::ComplexMap2D() {
    m_data = nullptr;
    m_width = 0;
    m_height = 0;
}

manta::ComplexMap2D::~ComplexMap2D() {
    assert(m_data == nullptr);
}

void manta::ComplexMap2D::initialize(int width, int height, math::real value) {
    m_width = width;
    m_height = height;

    m_data = StandardAllocator::Global()->allocate<math::Complex>(m_width * m_height, 16);

    for (int i = 0; i < m_width * m_height; i++) {
        m_data[i] = { (math::real)0.0, (math::real)0.0 };
    }
}

void manta::ComplexMap2D::destroy() {
    StandardAllocator::Global()->aligned_free(m_data, m_width * m_height);

    m_data = nullptr;
    m_width = 0;
    m_height = 0;
}

manta::math::Complex manta::ComplexMap2D::get(int u, int v) const {
    assert(m_data != nullptr);

    if (u < 0 || u >= m_width) return math::Complex();
    if (v < 0 || v >= m_height) return math::Complex();

    return m_data[v * m_width + u];
}

void manta::ComplexMap2D::set(const math::Complex &value, int u, int v) {
    assert(m_data != nullptr);

    m_data[v * m_width + u] = value;
}

manta::math::Complex manta::ComplexMap2D::sampleDiscrete(math::real_d ku, math::real_d kv) const {
    assert(m_data != nullptr);

    // Very simple sampling for now
    const int ik_x = (int)(ku + 0.5);
    const int ik_y = (int)(kv + 0.5);

    return m_data[ik_y * m_width + ik_x];
}

void manta::ComplexMap2D::fillByteBuffer(ImageByteBuffer *target, bool realOnly, const Margins *margins) const {
    int startX, startY;
    int width, height;

    if (margins != nullptr) {
        width = margins->width;
        height = margins->height;
        startX = margins->left;
        startY = margins->top;
    }
    else {
        width = m_width;
        height = m_height;
        startX = 0;
        startY = 0;
    }

    target->initialize(width, height);

    for (int u = 0; u < width; u++) {
        for (int v = 0; v < height; v++) {
            ImageByteBuffer::Color color;
            math::Complex value = get(u + startX, v + startY);

            if (realOnly) {
                target->convertToColor(math::loadVector((math::real)value.r, (math::real)0.0, (math::real)value.i), true, &color);
            }
            else {
                target->convertToColor(math::loadVector((math::real)value.r, (math::real)value.r, (math::real)value.r), true, &color);
            }
            target->setPixel(v, u, color);
        }
    }
}

void manta::ComplexMap2D::roll(ComplexMap2D *target) const {
    target->initialize(m_width, m_height);

    const int offsetX = m_width / 2;
    const int offsetY = m_height / 2;

    for (int i = 0; i < m_width; i++) {
        for (int j = 0; j < m_height; j++) {
            math::Complex v = get(i, j);
            target->set(v, (i + offsetX) % m_width, (j + offsetY) % m_height);
        }
    }
}

void manta::ComplexMap2D::fft(ComplexMap2D *target) const {
    target->initialize(m_width, m_height);

    const int width = m_width;
    const int height = m_height;
    const int minSpace = (width > height) ? width : height;

    math::Complex *inputBuffer = StandardAllocator::Global()->allocate<math::Complex>(minSpace);
    math::Complex *outputBuffer = StandardAllocator::Global()->allocate<math::Complex>(minSpace);

    // Transform the rows
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            math::Complex val = get(i, j);
            inputBuffer[i] = val;
        }

        NaiveFFT::fft(inputBuffer, outputBuffer, width);
        for (int i = 0; i < width; i++) {
            math::Complex result;
            result = outputBuffer[i];

            target->set(result, i, j);
        }
    }

    // Transform the columns
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            math::Complex val = target->get(i, j);
            inputBuffer[j] = val;
        }

        NaiveFFT::fft(inputBuffer, outputBuffer, height);
        for (int j = 0; j < height; j++) {
            math::Complex result;
            result = outputBuffer[j];

            target->set(result, i, j);
        }
    }

    StandardAllocator::Global()->free(inputBuffer, minSpace);
    StandardAllocator::Global()->free(outputBuffer, minSpace);
}

void manta::ComplexMap2D::fft_multithreaded(ComplexMap2D *target, int threadCount, bool inverse) const {
    target->initialize(m_width, m_height);

    int divVertical = m_height / threadCount;
    int divHorizontal = m_width / threadCount;

    if (divVertical == 0) divVertical = m_height;
    if (divHorizontal == 0) divHorizontal = m_width;

    std::thread **threads = StandardAllocator::Global()->allocate<std::thread *>(threadCount);

    // Horizontal FFTs
    for (int i = 0; i < threadCount; i++) {
        const int start = i * divVertical;
        int end = (i + 1) * divVertical;

        if (end >= m_height || i == (threadCount - 1)) {
            end = m_height;
        }

        if (start < end) {
            threads[i] = new std::thread(&ComplexMap2D::fftHorizontal, this, target, inverse, start, end);
        }
        else {
            threads[i] = nullptr;
        }
    }

    // Wait for all threads to complete
    for (int i = 0; i < threadCount; i++) {
        if (threads[i] != nullptr) {
            threads[i]->join();
            delete threads[i];
        }
    }

    // Vertical FFTs
    for (int i = 0; i < threadCount; i++) {
        const int start = i * divHorizontal;
        int end = (i + 1) * divHorizontal;

        if (end >= m_width || i == (threadCount - 1)) {
            end = m_width;
        }

        if (start < end) {
            threads[i] = new std::thread(&ComplexMap2D::fftVertical, this, target, inverse, start, end);
        }
        else {
            threads[i] = nullptr;
        }
    }

    // Wait for all threads to complete
    for (int i = 0; i < threadCount; i++) {
        if (threads[i] != nullptr) {
            threads[i]->join();
            delete threads[i];
        }
    }

    // Free temporary memory
    StandardAllocator::Global()->free(threads, threadCount);
}

void manta::ComplexMap2D::fftHorizontal(ComplexMap2D *target, bool inverse, int startRow, int endRow) const {
    math::Complex *inputBuffer = new math::Complex[m_width];
    math::Complex *outputBuffer = new math::Complex[m_width];

    for (int j = startRow; j < endRow; j++) {
        for (int i = 0; i < m_width; i++) {
            math::Complex val = get(i, j);
            inputBuffer[i] = val;
        }

        if (!inverse) NaiveFFT::fft(inputBuffer, outputBuffer, m_width);
        else NaiveFFT::fft_inverse(inputBuffer, outputBuffer, m_width);
        for (int i = 0; i < m_width; i++) {
            math::Complex result;
            result = outputBuffer[i];

            target->set(result, i, j);
        }
    }

    delete[] inputBuffer;
    delete[] outputBuffer;
}

void manta::ComplexMap2D::fftVertical(ComplexMap2D *target, bool inverse, int startColumn, int endColumn) const {
    math::Complex *inputBuffer = new math::Complex[m_height];
    math::Complex *outputBuffer = new math::Complex[m_height];

    for (int i = startColumn; i < endColumn; i++) {
        for (int j = 0; j < m_height; j++) {
            math::Complex val = target->get(i, j);
            inputBuffer[j] = val;
        }

        if (!inverse) NaiveFFT::fft(inputBuffer, outputBuffer, m_height);
        else NaiveFFT::fft_inverse(inputBuffer, outputBuffer, m_height);
        for (int j = 0; j < m_height; j++) {
            math::Complex result;
            result = outputBuffer[j];

            target->set(result, i, j);
        }
    }

    delete[] inputBuffer;
    delete[] outputBuffer;
}

void manta::ComplexMap2D::inverseFft(ComplexMap2D *target) const {
    target->initialize(m_width, m_height);

    int width = m_width;
    int height = m_height;
    int minSpace = (width > height) ? width : height;

    math::Complex *inputBuffer = StandardAllocator::Global()->allocate<math::Complex>(minSpace);
    math::Complex *outputBuffer = StandardAllocator::Global()->allocate<math::Complex>(minSpace);

    // Transform the rows
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            math::Complex val = get(i, j);
            inputBuffer[i] = val;
        }

        NaiveFFT::fft_inverse(inputBuffer, outputBuffer, width);
        for (int i = 0; i < width; i++) {
            math::Complex result;
            result = outputBuffer[i];

            target->set(result, i, j);
        }
    }

    // Transform the columns
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            math::Complex val = target->get(i, j);
            inputBuffer[j] = val;
        }

        NaiveFFT::fft_inverse(inputBuffer, outputBuffer, height);
        for (int j = 0; j < height; j++) {
            math::Complex result;
            result = outputBuffer[j];

            target->set(result, i, j);
        }
    }

    StandardAllocator::Global()->free(inputBuffer, minSpace);
    StandardAllocator::Global()->free(outputBuffer, minSpace);
}

void manta::ComplexMap2D::cft(ComplexMap2D *target, math::real_d physicalWidth, math::real_d physicalHeight) const {
    target->initialize(m_width, m_height);

    const int horizontalSamples = m_width;
    const int verticalSamples = m_height;

    math::real_d w_inv = 1 / physicalWidth;
    math::real_d h_inv = 1 / physicalHeight;

    math::real_d fs_x = horizontalSamples * w_inv;
    math::real_d fs_y = verticalSamples * h_inv;

    math::Complex fs_inv_s = math::Complex(1 / (fs_x * fs_y), (math::real_d)0.0);

    const int halfHorizontalSamples = horizontalSamples / 2;
    const int halfVerticalSamples = verticalSamples / 2;

    for (int kx = -halfHorizontalSamples; kx < halfHorizontalSamples; kx++) {
        // frequency_x = kx / physicalWidth

        for (int ky = -halfVerticalSamples; ky < halfVerticalSamples; ky++) {
            // frequency_y = ky / physicalHeight

            // phase = exp(2 * pi * (frequency_x * (w / 2) + frequency_y * (h / 2)))
            const int phase = kx + ky;

            math::Complex phaseTransformation;
            phaseTransformation.r = (phase % 2 == 0) ? (math::real_d)1.0 : (math::real_d)-1.0;
            phaseTransformation.i = (math::real_d)0.0;

            const int mapIndexX = (kx + horizontalSamples) % horizontalSamples;
            const int mapIndexY = (ky + verticalSamples) % verticalSamples;

            math::Complex dftApprox = get(mapIndexX, mapIndexY);
            dftApprox = dftApprox * phaseTransformation * fs_inv_s;

            target->set(dftApprox, mapIndexX, mapIndexY);
        }
    }
}

void manta::ComplexMap2D::inverseCft(ComplexMap2D *target, math::real_d physicalWidth, math::real_d physicalHeight) const {
    target->initialize(m_width, m_height);

    const int horizontalSamples = m_width;
    const int verticalSamples = m_height;

    math::real_d w_inv = 1 / physicalWidth;
    math::real_d h_inv = 1 / physicalHeight;

    math::real_d fs_x = horizontalSamples * w_inv;
    math::real_d fs_y = verticalSamples * h_inv;

    math::Complex fs_inv_s = math::Complex(1 / (fs_x * fs_y), (math::real_d)0.0);

    const int halfHorizontalSamples = horizontalSamples / 2;
    const int halfVerticalSamples = verticalSamples / 2;

    for (int kx = -halfHorizontalSamples; kx < halfHorizontalSamples; kx++) {
        // frequency_x = kx / physicalWidth

        for (int ky = -halfVerticalSamples; ky < halfVerticalSamples; ky++) {
            // frequency_y = ky / physicalHeight

            // phase = exp(2 * pi * (frequency_x * (w / 2) + frequency_y * (h / 2)))
            const int phase = kx + ky;

            math::Complex phaseTransformation;
            phaseTransformation.r = (phase % 2 == 0) ? (math::real_d)1.0 : (math::real_d) -1.0;
            phaseTransformation.i = (math::real_d)0.0;

            const int mapIndexX = (kx + horizontalSamples) % horizontalSamples;
            const int mapIndexY = (ky + verticalSamples) % verticalSamples;

            math::Complex dftApprox = get(mapIndexX, mapIndexY);
            dftApprox = dftApprox / (phaseTransformation * fs_inv_s);

            target->set(dftApprox, mapIndexX, mapIndexY);
        }
    }
}

void manta::ComplexMap2D::cftConvolve(ComplexMap2D *gDft, math::real_d physicalWidth, math::real_d physicalHeight) {
    const int horizontalSamples = m_width;
    const int verticalSamples = m_height;

    const math::real_d w_inv = 1 / physicalWidth;
    const math::real_d h_inv = 1 / physicalHeight;

    const math::real_d fs_x = horizontalSamples * w_inv;
    const math::real_d fs_y = verticalSamples * h_inv;

    const math::Complex fs_inv_s = math::Complex(1 / (fs_x * fs_y), (math::real_d)0.0);

    const int halfHorizontalSamples = horizontalSamples / 2;
    const int halfVerticalSamples = verticalSamples / 2;

    for (int kx = -halfHorizontalSamples; kx < halfHorizontalSamples; kx++) {
        // frequency_x = kx / physicalWidth

        for (int ky = -halfVerticalSamples; ky < halfVerticalSamples; ky++) {
            // frequency_y = ky / physicalHeight

            // phase = exp(2 * pi * (frequency_x * (w / 2) + frequency_y * (h / 2)))
            int phase = kx + ky;

            math::Complex phaseTransformation;
            phaseTransformation.r = (phase % 2 == 0) ? (math::real_d)1.0 : (math::real_d) -1.0;
            phaseTransformation.i = (math::real_d)0.0;

            int mapIndexX = (kx + horizontalSamples) % horizontalSamples;
            int mapIndexY = (ky + verticalSamples) % verticalSamples;

            math::Complex dftApprox = get(mapIndexX, mapIndexY) * gDft->get(mapIndexX, mapIndexY);
            dftApprox = fs_inv_s * dftApprox * phaseTransformation;

            set(dftApprox, mapIndexX, mapIndexY);
        }
    }
}

void manta::ComplexMap2D::magnitude() {
    for (int i = 0; i < m_width; i++) {
        for (int j = 0; j < m_height; j++) {
            math::Complex v = get(i, j);
            v = v * v.conjugate();
            set(v, i, j);
        }
    }
}

void manta::ComplexMap2D::multiply(const math::Complex &s) {
    for (int i = 0; i < m_width; i++) {
        for (int j = 0; j < m_height; j++) {
            math::Complex v = get(i, j);
            set(v * s, i, j);
        }
    }
}

void manta::ComplexMap2D::resizeSafe(ComplexMap2D *target, Margins *margins) const {
    int minWidth = m_width * 2;
    int minHeight = m_height * 2;

    int width = 1, height = 1;
    while (width < minWidth) width *= 2;
    while (height < minHeight) height *= 2;

    int marginX = (width - m_width) / 2;
    int marginY = (height - m_height) / 2;

    margins->height = m_height;
    margins->width = m_width;
    margins->left = marginX;
    margins->top = marginY;

    target->initialize(width, height);

    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            if (i >= marginX && (i - marginX) < m_width) {
                if (j >= marginY && (j - marginY) < m_height) {
                    target->set(get(i - marginX, j - marginY), i, j);
                }
            }
        }
    }
}

void manta::ComplexMap2D::resize(int width, int height, ComplexMap2D *target, Margins *margins) const {
    int marginX = (width - m_width) / 2;
    int marginY = (height - m_height) / 2;

    target->initialize(width, height);

    if (margins != nullptr) {
        margins->height = height;
        margins->width = width;
        margins->left = marginX;
        margins->top = marginY;
    }

    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            if (i >= marginX && (i - marginX) < m_width) {
                if (j >= marginY && (j - marginY) < m_height) {
                    target->set(get(i - marginX, j - marginY), i, j);
                }
            }
        }
    }
}

void manta::ComplexMap2D::multiply(const ComplexMap2D *b) {
    for (int i = 0; i < m_width; i++) {
        for (int j = 0; j < m_height; j++) {
            set(get(i, j) * b->get(i, j), i, j);
        }
    }
}

void manta::ComplexMap2D::copy(const ComplexMap2D *b) {
    initialize(b->getWidth(), b->getHeight());

    for (int i = 0; i < m_width; i++) {
        for (int j = 0; j < m_height; j++) {
            set(b->get(i, j), i, j);
        }
    }
}

void manta::ComplexMap2D::copy(const RealMap2D *b) {
    initialize(b->getWidth(), b->getHeight());

    for (int i = 0; i < m_width; i++) {
        for (int j = 0; j < m_height; j++) {
            set(math::Complex(b->get(i, j), (math::real)0.0), i, j);
        }
    }
}

void manta::ComplexMap2D::copy(const ImagePlane *b, int channel) {
    initialize(b->getWidth(), b->getHeight());

    for (int i = 0; i < m_width; i++) {
        for (int j = 0; j < m_height; j++) {
            set(math::Complex(math::get(b->sample(i, j), channel), (math::real)0.0), i, j);
        }
    }
}

void manta::ComplexMap2D::copy(const VectorMap2D *b, int channel) {
    initialize(b->getWidth(), b->getHeight());

    for (int i = 0; i < m_width; i++) {
        for (int j = 0; j < m_height; j++) {
            set(math::Complex(math::get(b->get(i, j), channel), (math::real)0.0), i, j);
        }
    }
}
