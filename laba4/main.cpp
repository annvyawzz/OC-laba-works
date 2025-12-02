#include "Browser.h"
#include "Downloader.h"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc == 1) {
        // Browser mode
        Browser browser;
        if (browser.Initialize()) {
            browser.Run();
        }
    }
    else {
        // Downloader mode
        Downloader downloader;
        if (downloader.Initialize()) {
            downloader.Run();
        }
        else {
            std::cout << "Downloader failed to initialize sync objects" << std::endl;
        }
    }

    return 0;
}