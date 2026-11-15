#ifndef CYNE_ROUTE_H
#define CYNE_ROUTE_H

#include "../error.h"
#include "../message.h"

typedef struct Route {
    char* rule;
    void (*view_func)(HttpRequest* req, HttpResponse* out_res);
} Route;

typedef struct RouteDynArr {
   Route* arr;
   size_t size;
   size_t capacity;
} RouteDynArr;

CyneResult RouteDynArr_init(size_t init_capacity, RouteDynArr** out);
CyneResult RouteDynArr_free(RouteDynArr* darr);
CyneResult RouteDynArr_index(RouteDynArr* darr, const char* rule, size_t* out);
CyneResult RouteDynArr_insert(RouteDynArr* darr, const char* rule, void (*view_func)(HttpRequest*, HttpResponse*));
CyneResult RouteDynArr_remove(RouteDynArr* darr, const char* rule);

typedef struct RouteMap {
    size_t size;
    RouteDynArr* routes[10];  // one slot for each HTTP method
} RouteMap;

CyneResult RouteMap_init(RouteMap** out);
CyneResult RouteMap_free(RouteMap* rm);
CyneResult RouteMap_set(RouteMap* rm, HttpMethod method, const char* rule, void (*view_func)(HttpRequest*, HttpResponse*));
CyneResult RouteMap_remove(RouteMap* rm, HttpMethod method, const char* rule);
CyneResult RouteMap_get(RouteMap* rm, HttpMethod method, const char* rule, Route* out);
CyneResult RouteMap_route(RouteMap* rm, const HttpRequest* req, HttpResponse* out_res);

/* End of header */
#endif