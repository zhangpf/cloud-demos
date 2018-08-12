// Copyright (c) 2018 Pengfei Zhang <zpfalpc23@gmail.com>

// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:

// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "blogstore.capnp.h"
#include <capnp/ez-rpc.h>
#include <chrono>
#include <iostream>
#include <kj/exception.h>
#include <map>
#include <string>

#define TEXT_LEN 4096
#define BLOG_COUNT 1024

class Timer {
public:
    Timer()
        : m_beg(clock_::now()) {
    }
    void reset() {
        m_beg = clock_::now();
    }

    double elapsed() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   clock_::now() - m_beg)
            .count();
    }

private:
    typedef std::chrono::high_resolution_clock clock_;
    typedef std::chrono::duration<double, std::ratio<1>> second_;
    std::chrono::time_point<clock_> m_beg;
};

std::string generateRandomText() {
    std::string text("");
    for (int i = 0; i < TEXT_LEN; i++) {
        text += rand() % 26 + 'A';
    }
    return text;
}

void remoteStore(BlogStore::Client& blogStore,
                 kj::WaitScope& waitScope,
                 uint64_t key,
                 const std::string& blog) {
    // Set up the request.
    auto request = blogStore.storeRequest();
    request.setKey(key);
    request.getBlog().setBlog(blog);

    // Send it, which returns a promise for the result (without blocking).
    auto storePromise = request.send();

    storePromise.wait(waitScope);
}

// The get may throw exception when the key is not existing.
std::string remoteGet(BlogStore::Client& blogStore,
                      kj::WaitScope& waitScope,
                      uint64_t key) {
    // Set up the request.
    auto request = blogStore.getRequest();
    request.setKey(key);

    // Send it, which returns a promise for the result (without blocking).
    auto getPromise = request.send();

    auto readPromise = getPromise.getBlog().readRequest().send();

    auto response = readPromise.wait(waitScope);

    return response.getBlog();
}

// The remove may throw exception when the key is not existing.
void remoteRemove(BlogStore::Client& blogStore,
                  kj::WaitScope& waitScope,
                  uint64_t key) {
    // Set up the request.
    auto request = blogStore.removeRequest();
    request.setKey(key);

    // Send it, which returns a promise for the result (without blocking).
    auto removePromise = request.send();

    removePromise.wait(waitScope);
}

// The copy may throw exception when the source key is not existing.
void remoteCopy(BlogStore::Client& blogStore,
                kj::WaitScope& waitScope,
                uint64_t key1,
                uint64_t key2) {
    // Set up the request.
    auto getRequest = blogStore.getRequest();
    getRequest.setKey(key1);

    // Send it, which returns a promise for the result (without blocking).
    auto getResult = getRequest.send().getBlog();

    auto storeRequest = blogStore.storeRequest();
    storeRequest.setKey(key2);
    storeRequest.getBlog().setPreviousGet(getResult);

    // Send it, which returns a promise for the result (without blocking).
    auto storePromise = storeRequest.send();

    storePromise.wait(waitScope);
}

int main(int argc, const char* argv[]) {
    if (argc != 2) {
        std::cerr << "usage: " << argv[0] << " HOST:PORT\n"
                  << std::endl;
        return 1;
    }

    srand(time(NULL));
    capnp::EzRpcClient client(argv[1]);
    BlogStore::Client blogStore = client.getMain<BlogStore>();

    Timer timer;

    // Keep an eye on `waitScope`.  Whenever you see it used is a place where we
    // stop and wait for the server to respond.  If a line of code does not use
    // `waitScope`, then it does not block!
    auto& waitScope = client.getWaitScope();

    std::map<uint64_t, std::string> localBlogs;

    // Generate random text.
    {
        for (int i = 0; i < BLOG_COUNT; i++) {
            localBlogs[i] = generateRandomText();
        }
    }

    // Store all the 1024 blogs
    {
        std::cout << "Store all the " << BLOG_COUNT << " blogs... ";
        timer.reset();

        for (int i = 0; i < BLOG_COUNT; i++) {
            remoteStore(blogStore, waitScope, i, localBlogs[i]);
        }

        double elapsed = timer.elapsed();
        std::cout << "Done and success! Time costed: " << elapsed << "ms." << std::endl;
    }

    // Get and check all the 1024 blogs
    {
        std::cout << "Get and check all the " << BLOG_COUNT << " blogs...";
        timer.reset();

        for (int i = 0; i < BLOG_COUNT; i++) {
            try {
                auto blog = remoteGet(blogStore, waitScope, i);
                if (blog != localBlogs[i]) {
                    std::cerr << "The result of Get is wrong!!!" << std::endl;
                    std::exit(1);
                }
            } catch (kj::Exception const& e) {
                std::cerr << e.getDescription().cStr() << std::endl;
            }
        }

        double elapsed = timer.elapsed();
        std::cout << "Done and success! Time costed: " << elapsed << "ms." << std::endl;
    }

    {
        std::cout << "Test for getting a non-existing blog (key == "
                  << BLOG_COUNT << ")....";

        try {
            remoteGet(blogStore, waitScope, BLOG_COUNT);
        } catch (kj::Exception const& e) {
            std::cerr << e.getDescription().cStr() << std::endl;
        }
    }

    {
        std::cout << "Copy all the " << BLOG_COUNT
                  << " blogs (from i to i + " << BLOG_COUNT
                  << ")... ";
        timer.reset();

        for (int i = 0; i < BLOG_COUNT; i++) {
            try {
                remoteCopy(blogStore, waitScope, i, i + BLOG_COUNT);
            } catch (kj::Exception const& e) {
                std::cerr << e.getDescription().cStr() << std::endl;
            }
        }

        double elapsed = timer.elapsed();
        std::cout << "Done and success! Time costed: " << elapsed << "ms." << std::endl;

        std::cout << "Check all the new " << BLOG_COUNT << " blogs... ";

        for (int i = 0; i < BLOG_COUNT; i++) {
            try {
                auto blog = remoteGet(blogStore, waitScope, i + BLOG_COUNT);
                if (blog != localBlogs[i]) {
                    std::cerr << "The result of Get is wrong!!!" << std::endl;
                    std::exit(1);
                }
            } catch (kj::Exception const& e) {
                std::cerr << e.getDescription().cStr() << std::endl;
            }
        }
        std::cout << "Done and success!" << std::endl;
    }

    {
        std::cout << "Remove all the " << 2 * BLOG_COUNT << " blogs...";
        timer.reset();

        for (int i = 0; i < 2 * BLOG_COUNT; i++) {
            try {
                remoteRemove(blogStore, waitScope, i);
            } catch (kj::Exception const& e) {
                std::cerr << e.getDescription().cStr() << std::endl;
            }
        }

        double elapsed = timer.elapsed();
        std::cout << "Done and success! Time costed: " << elapsed << "ms." << std::endl;
    }

    {
        std::cout << "Again, try to get a non-existing blog (key == 0)...";
        try {
            remoteGet(blogStore, waitScope, 0);
        } catch (kj::Exception const& e) {
            std::cerr << e.getDescription().cStr() << std::endl;
        }
    }
    return 0;
}
