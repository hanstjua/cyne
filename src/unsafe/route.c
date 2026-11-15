#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../error.h"
#include "../url.h"
#include "route.h"

typedef struct UrlVarSet {
   char vars[(int)(MAX_ENDPOINT_VARIABLE_COUNT / 0.7f)][MAX_ENDPOINT_VARIABLE_NAME_LEN];
   size_t max_size;
} UrlVarSet;

void UrlVarSet_init(UrlVarSet* set) {
   size_t capacity = MAX_ENDPOINT_VARIABLE_COUNT / 0.7f;
   for (size_t i=0; i<capacity; i++) {
     set->vars[i][0] = 0;
   }
   set->max_size = capacity;
}

uint64_t UrlVarSet_hash(const char* s) {
   // DJB hash
   uint64_t h = 5381;
   while (*s != 0) {
      h = h * 33 + *(s++);
   }
   return h;
}

bool UrlVarSet_has(const UrlVarSet* set, const char* name) {
   uint64_t hash = UrlVarSet_hash(name) % set->max_size;
   for (size_t i=0; i<set->max_size; i++) {
      if (strcmp(set->vars[(hash+i)%set->max_size], name) == 0) {
         return true;
      }
   }
   return false;
}

void UrlVarSet_add(UrlVarSet* set, const char* name) {
   uint64_t hash = UrlVarSet_hash(name) % set->max_size;
   for (size_t i=0; i<set->max_size; i++) {
      if (strlen(set->vars[(hash+i)%set->max_size]) > 0) {
         if (strcmp(set->vars[(hash+i)%set->max_size], name) == 0) {
            return;
         }
      } else {
         snprintf(set->vars[(hash+i)%set->max_size], strlen(name) + 1, "%s", name);
      }   
   }

}

CyneResult RouteDynArr_init(size_t init_capacity, RouteDynArr** out) {
   *out = malloc(sizeof(RouteDynArr));
   if (*out == NULL) {
      return OUT_OF_MEMORY;
   }

   (*out)->arr = malloc(sizeof(Route) * init_capacity);
   if ((*out)->arr == NULL) {
      return OUT_OF_MEMORY;
   }

   (*out)->size = 0;
   (*out)->capacity = init_capacity;

   return SUCCESS;
}

CyneResult RouteDynArr_free(RouteDynArr* darr) {
   free(darr->arr);
   darr->arr = NULL;
   free(darr);
   return SUCCESS;
}

CyneResult RouteDynArr_index(RouteDynArr* darr, const char* rule, size_t* out) {
   *out = 0;
   RuleToken target_tokens[MAX_ENDPOINT_VARIABLE_COUNT];
   RuleToken current_tokens[MAX_ENDPOINT_VARIABLE_COUNT];
   size_t target_tokens_size;
   size_t current_tokens_size;
   char* current_literal;
   char* target_literal;
   size_t current_literal_len;
   size_t target_literal_len;
   bool match = false;
   UrlVarSet url_var_set;

   CyneResult result = tokenize_rule(rule, target_tokens, &target_tokens_size);
   if (result != SUCCESS) {
      return result;
   }

   // check for duplicate url var
   UrlVarSet_init(&url_var_set);
   for (size_t i=0; i<target_tokens_size; i++) {
      if (target_tokens[i].type != UNKNOWN_TOK && target_tokens[i].type != LITERAL_TOK) {
         if (UrlVarSet_has(&url_var_set, target_tokens[i].name)) {
            return DUPLICATE_ENDPOINT_VARIABLES;
         } else {
            UrlVarSet_add(&url_var_set, target_tokens[i].name);
         }
      }
   }

   for (size_t i=0; i<darr->size; i++) {
      result = tokenize_rule(darr->arr[i].rule, current_tokens, &current_tokens_size);
      if (result != SUCCESS) {
         return result;
      }

      if (current_tokens_size != target_tokens_size) {
         continue;
      }

      for (size_t j=0; j<current_tokens_size; j++) {
         match = current_tokens[j].type == target_tokens[j].type;
         if (match) {
            switch (current_tokens[j].type) {
               case LITERAL_TOK:
               current_literal = darr->arr[i].rule + current_tokens[j].rule_index.start;
               current_literal_len = current_tokens[j].rule_index.end - current_tokens[j].rule_index.start;
               target_literal = rule + target_tokens[j].rule_index.start;
               target_literal_len = target_tokens[j].rule_index.end - target_tokens[j].rule_index.start;
               match = current_literal_len == target_literal_len && strncmp(current_literal, target_literal, target_tokens[j].rule_index.end - target_tokens[j].rule_index.start) == 0;
               break;

               default:
               match = strcmp(current_tokens[j].name, target_tokens[j].name) == 0;
               break;
            }
         } else {
            break;
         }
      }

      if (match) {
         *out = i;
         return SUCCESS;
      }
   }
   return NOT_FOUND;
}

CyneResult RouteDynArr_insert(RouteDynArr* darr, const char* rule, void (*view_func)(HttpRequest*, HttpResponse*)) {
   size_t route_index;
   if (RouteDynArr_index(darr, rule, &route_index) == NOT_FOUND) {
      route_index = darr->size;
   }

   if (route_index == darr->capacity) {
      size_t new_capacity = darr->capacity * 1.5;
      Route* new_arr = malloc(sizeof(Route) * new_capacity);
      if (new_arr == NULL) {
         return OUT_OF_MEMORY;
      }
      
      for (int i=0; i<darr->size; i++) {
         new_arr[i] = darr->arr[i];
      }

      free(darr->arr);
      darr->arr = new_arr;
      darr->capacity = new_capacity;
   }

   // TODO: insert new route into sorted order

   darr->arr[route_index].rule = rule;
   darr->arr[route_index].view_func = view_func;
   darr->size += route_index == darr->size ? 1 : 0;

   return SUCCESS;
}

CyneResult RouteDynArr_remove(RouteDynArr* darr, const char* rule) {
   for (int i=0; i<darr->size; i++) {
      if (strcmp(darr->arr[i].rule, rule) == 0) {
         for (int j=i; j<darr->size-1; j++) {
            darr->arr[j] = darr->arr[j+1];
         }
         darr->size--;
         break;
      }
   }
   return SUCCESS;
}

CyneResult RouteMap_init(RouteMap** out) {
   RouteDynArr* darr;
   CyneResult result;

   *out = malloc(sizeof(RouteMap));
   if (*out == NULL) {
      return OUT_OF_MEMORY;
   }

   for (int i=0; i<10; i++) {
      result = RouteDynArr_init(8, &darr);
      if (result != SUCCESS) {
         return result;
      }
      (*out)->routes[i] = darr;
   }
   (*out)->size = 0;

   return SUCCESS;
}

CyneResult RouteMap_free(RouteMap* rm) {
   for (int i=0; i<10; i++) {
      RouteDynArr_free(rm->routes[i]);
      rm->routes[i] = NULL;
   }
   rm->size = 0;
   free(rm);

   return SUCCESS;
}

CyneResult RouteMap_set(RouteMap* rm, HttpMethod method, const char* rule, void (*view_func)(HttpRequest* req, HttpResponse* res)) {
   size_t index;
   bool already_exists;
   CyneResult result = RouteDynArr_index(rm->routes[method], rule, &index);
   if (result != SUCCESS && result != NOT_FOUND) {
      return result;
   }
   
   // Modifying existing route
   already_exists = result != NOT_FOUND;

   result = RouteDynArr_insert(rm->routes[method], rule, view_func);
   if (result != SUCCESS) {
      return result;
   }

   rm->size += already_exists ? 0 : 1;

   return SUCCESS;
}

CyneResult RouteMap_get(RouteMap* rm, HttpMethod method, const char* rule, Route* out) {
   // TODO: Figure out how to treat <name> and <s:name> as the same token in rule (e.g. /url/<name> vs /url/<s:name>)
   size_t index;
   RouteDynArr* darr = rm->routes[method];
   CyneResult result = RouteDynArr_index(darr, rule, &index);
   if (result != SUCCESS) {
      return result;
   }

   *out = darr->arr[index];

   return SUCCESS;
}

CyneResult RouteMap_remove(RouteMap* rm, HttpMethod method, const char* rule) {
   // TODO: Figure out how to treat <name> and <s:name> as the same token in rule (e.g. /url/<name> vs /url/<s:name>)
   size_t index;
   bool exists = RouteDynArr_index(rm->routes[method], rule, &index) != NOT_FOUND;
   
   CyneResult result = RouteDynArr_remove(rm->routes[method], rule);
   if (result != SUCCESS) {
      return result;
   }

   rm->size -= exists ? 1 : 0;

   return SUCCESS;
}

CyneResult RouteMap_route(RouteMap* rm, const HttpRequest* req, HttpResponse* out_res) {
   RouteDynArr* routes = rm->routes[req->method];
   CyneResult result;
   EndpointVariable vars[MAX_ENDPOINT_VARIABLE_COUNT];
   bool match;

   for (int i=0; i<routes->size; i++) {
      result = parse_endpoint(req->request_target, routes->arr[i].rule, &match, vars);
      if (result != SUCCESS) {
         return result;
      }

      if (match) {
         routes->arr[i].view_func(req, out_res);
         break;
      }
   }
   return SUCCESS;
}