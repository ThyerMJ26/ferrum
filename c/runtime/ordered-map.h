struct OrderedMap;

typedef struct OrderedMap *OrderedMapPtr;

OrderedMapPtr omap_init();

// void omap_set(OrderedMapPtr om, Ref key, Ref val);
// Ref omap_get(OrderedMapPtr om, Ref key);
// void omap_erase(OrderedMapPtr om, Ref key);

void omap_set(OrderedMapPtr om, Any key, Any val);
Any omap_get(OrderedMapPtr om, Any key);
void omap_erase(OrderedMapPtr om, Any key);


OrderedMapPtr omap_copy(OrderedMapPtr om);
