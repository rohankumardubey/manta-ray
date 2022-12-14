#include "utils.h"

#include "../include/image_handling.h"
#include "../include/os_utilities.h"
#include "settings.h"
#include "../include/image_byte_buffer.h"
#include "../include/jpeg_writer.h"

#include <sstream>
#include <time.h>
#include <iostream>

void manta_demo::createAllDirectories() {
    std::string baseRenderPath = std::string(RENDER_OUTPUT);
    std::string baseWorkspacePath = std::string(WORKSPACE_PATH);

    manta::createFolder(baseWorkspacePath.c_str());
    manta::createFolder((baseWorkspacePath + "diagnostics").c_str());

    manta::createFolder(baseRenderPath.c_str());
    manta::createFolder(baseRenderPath.c_str());
    manta::createFolder((baseRenderPath + "bitmap").c_str());
    manta::createFolder((baseRenderPath + "info").c_str());
    manta::createFolder((baseRenderPath + "raw").c_str());
}

manta::math::Vector manta_demo::getColor(int r, int g, int b, manta::math::real gamma) {
    manta::math::real rr = r / (manta::math::real)255.0;
    manta::math::real rg = g / (manta::math::real)255.0;
    manta::math::real rb = b / (manta::math::real)255.0;

    rr = pow(rr, gamma);
    rg = pow(rg, gamma);
    rb = pow(rb, gamma);

    manta::math::Vector v = manta::math::loadVector(rr, rg, rb);
    return v;
}

std::string manta_demo::createUniqueRenderFilename(const char *jobName, int samples) {
    time_t rawTime;
    struct tm timeInfo;
    char buffer[256];

    time(&rawTime);
    localtime_s(&timeInfo, &rawTime);

    strftime(buffer, 256, "%F_T%H_%M_%S", &timeInfo);

    std::stringstream ss;
    ss << samples;

    return std::string(buffer) + "_" + std::string(jobName) + "_S" + ss.str();
}

void manta_demo::writeJpeg(const char *fname, manta::ImagePlane *sceneBuffer, int quality) {
    manta::ImageByteBuffer byteBuffer;
    byteBuffer.initialize(sceneBuffer, true);

    manta::JpegWriter writer;
    writer.setQuality(quality);
    writer.write(&byteBuffer, fname);

    byteBuffer.free();
}

void manta_demo::editImage(manta::ImagePlane *sceneBuffer, const std::string &outputFname) {
    manta::SaveImageData(sceneBuffer->getBuffer(), sceneBuffer->getWidth(), sceneBuffer->getHeight(), outputFname.c_str());

    // Create a temporary scene buffer
    manta::ImagePlane temp;

    std::cout << "**** Image Editing Utility ****" << std::endl;
    while (true) {
        char command;
        std::cout << "Edit image? (y/n) ";
        std::cin >> command;
        if (command == 'y' || command == 'Y') {
            temp.copyFrom(sceneBuffer);

            manta::math::real scale;

            std::cout << "Enter scale factor: ";
            std::cin >> scale;

            //temp.scale(scale);
            manta::SaveImageData(temp.getBuffer(), temp.getWidth(), temp.getHeight(), outputFname.c_str());

            temp.destroy();
        }
        else {
            break;
        }
    }
}
