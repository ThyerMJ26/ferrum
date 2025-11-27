
// TODO implement an associative container use C++ STL map (and/or unordered_map)

extern "C" {
    #include "runtime.h"
    #include "ordered-map.h"
}

#include <map>
#include <gc_cpp.h>
#include <gc/gc_allocator.h>


// struct RefLt {
//     bool operator() (const Ref& a, const Ref& b) const {
//         return refCompare(a, b) < 0;
//     }
// };

// struct OrderedMap : public gc {
//     std::map<Ref,Ref,RefLt, gc_allocator<Ref>> map;
// };

struct AnyLt {
    bool operator() (const Any& a, const Any& b) const {
        return any_compare(a, b) < 0;
    }
};

struct OrderedMap : public gc {
    std::map<Any,Any,AnyLt, gc_allocator<Any>> map;
};



extern "C"
OrderedMapPtr omap_init() {
    return new OrderedMap;
}

extern "C"
void omap_set(OrderedMapPtr om, Any key, Any val) {
    // fprintf(stderr, "omap_set\n");
    om->map[key] = val;
}

extern "C"
void omap_erase(OrderedMapPtr om, Any key) {
    // fprintf(stderr, "omap_erase\n");
    om->map.erase(key);
}

extern "C"
Any omap_get(OrderedMapPtr om, Any key) {
    // fprintf(stderr, "omap_get\n");
    auto it = om->map.find(key);
    if (it == om->map.end()) {
        // return an "Any" with a NULL repr, 
        // this indicates that there is nothing here, not even a nil
        return (Any){};
    }
    else {
        return it->second;
    }
}

extern "C"
OrderedMapPtr omap_copy(OrderedMapPtr om) {
    OrderedMapPtr om2 = new OrderedMap();
    *om2 = *om;
    return om2;
}
