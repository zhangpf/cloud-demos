@0xf79af02aadd13d6d;

struct Date {
    year @0 :Int16;
    month @1 :UInt8;
    day @2 :UInt8;
}

struct Blog {
    data @0 :Date;
    author @1 :Text;
    title @2 :Text;
    body @3 :Text;
}

interface BlogStore {

    struct LoadResult {
        result :union {
            failure @0 :Text;
            blog @1 :Blog;
        }
    }

    load @0 (key :UInt64) -> (result :LoadResult);

    store @1 (key :UInt64, value :Blog) -> (result :Bool);

    remove @2 (key :UInt64);
}