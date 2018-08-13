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
#include <capnp/message.h>
#include <iostream>
#include <kj/debug.h>
#include <map>

kj::Promise<capnp::Text::Reader> readBlog(BlogStore::Blog::Client blog) {
    // Helper function to asynchronously call read() on a BlogStore::Blog and
    // return a promise for the result.  (In the future, the generated code might
    // include something like this automatically.)

    return blog.readRequest().send().then([](capnp::Response<BlogStore::Blog::ReadResults> result) {
        return result.getBlog();
    });
}

class BlogImpl final : public BlogStore::Blog::Server {
    // Simple implementation of the Calculator.Value Cap'n Proto interface.

public:
    BlogImpl(std::string blog)
        : blog(blog) {}

    kj::Promise<void> read(ReadContext context) {
        context.getResults().setBlog(blog);
        return kj::READY_NOW;
    }

private:
    std::string blog;
};

class BlogStoreImpl final : public BlogStore::Server {
    // Implementation of the BlogStore Cap'n Proto interface.

public:
    kj::Promise<void> get(GetContext context) override {
        auto key = context.getParams().getKey();

        auto find = storage.find(key);

        if (find == storage.end()) {
            KJ_FAIL_REQUIRE("blog entry for " + std::to_string(key) + " not found!");
        } else {
            context.getResults().setBlog(kj::heap<BlogImpl>(find->second));
            return kj::READY_NOW;
        }
    }

    kj::Promise<void> store(StoreContext context) override {
        auto key = context.getParams().getKey();
        auto blog = context.getParams().getBlog();

        // Tackle the two different cases.
        switch (blog.which()) {

        case BlogStore::Store::BLOG:
            storage[key] = blog.getBlog();
            return kj::READY_NOW;

        case BlogStore::Store::PREVIOUS_GET:
            return readBlog(blog.getPreviousGet()).then([ KJ_CPCAP(context), this, key ](kj::StringPtr blog) mutable {
                storage[key] = blog;
            });
        default:
            KJ_FAIL_REQUIRE("Unknown data type.");
        }
    }

    kj::Promise<void> remove(RemoveContext context) override {
        auto key = context.getParams().getKey();

        auto find = storage.find(key);

        if (find == storage.end()) {
            KJ_FAIL_REQUIRE("blog entry for " + std::to_string(key) + " not found!");
        } else {
            storage.erase(key);
            return kj::READY_NOW;
        }
    }

    BlogStoreImpl() = default;

private:
    std::map<uint64_t, std::string> storage;
};

int main(int argc, const char* argv[]) {
    if (argc != 2) {
        std::cerr << "usage: " << argv[0]
                  << " ADDRESS[:PORT]\n"
                     "Runs the server bound to the given address/port.\n"
                     "ADDRESS may be '*' to bind to all local addresses.\n"
                     ":PORT may be omitted to choose a port automatically."
                  << std::endl;
        return 1;
    }

    // Set up a server.
    capnp::EzRpcServer server(kj::heap<BlogStoreImpl>(), argv[1], 1234);

    // Write the port number to stdout, in case it was chosen automatically.
    auto& waitScope = server.getWaitScope();
    uint port = server.getPort().wait(waitScope);
    if (port == 0) {
        // The address format "unix:/path/to/socket" opens a unix domain socket,
        // in which case the port will be zero.
        std::cout << "Listening on Unix socket..." << std::endl;
    } else {
        std::cout << "Listening on port " << port << "..." << std::endl;
    }

    // Run forever, accepting connections and handling requests.
    kj::NEVER_DONE.wait(waitScope);
}
