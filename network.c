#include <stdlib.h>
#include <assert.h>
#include "agent.h"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

// Network of agents
typedef struct {
   Agent **adj_list;
   int n;
} Network;

// Generate float in range [0, 1)
float frand(void) {
   float f = (float)rand() / ((float)RAND_MAX + 1.0f);
   if (f >= 1.0f) {
      f = 0;
   }
   return f;
}

// Create a network of n isolated nodes
Network *create_network(int n) {
   Network *network = malloc(sizeof(Network));
   network->n = n;

   network->adj_list = malloc(n * sizeof(Agent*));
   for (int i = 0; i < n; i++) {
      network->adj_list[i] = create_agent();
   }

   return network;
}

// Check if network already contains edge
bool contains_edge(Network *network, int i, int j) {
   Agent *src  = network->adj_list[i],
         *dest = network->adj_list[j];

   for (int k = 0; k < src->degree; k++) {
      if (dest == src->neighbours[k]) {
         return true;
      }
   }
   return false;
}

// Insert undirect edge
void edge_insert(Network *network, int i, int j) {
   // No self loops
   assert(i != j);

   if (contains_edge(network, i, j)) {
      return;
   }

   Agent *src  = network->adj_list[i],
         *dest = network->adj_list[j];

   if (src->degree == src->capacity) {
      src->capacity *= 2;
      src->neighbours = realloc(src->neighbours, src->capacity * sizeof(Agent*));
   }
   src->degree++;
   src->neighbours[src->degree - 1] = dest;

   if (dest->degree == dest->capacity) {
      dest->capacity *= 2;
      dest->neighbours = realloc(dest->neighbours, dest->capacity * sizeof(Agent*));
   }
   dest->degree++;
   dest->neighbours[dest->degree - 1] = src;
}

// Connect all nodes in a new network
void make_complete(Network *network) {
   for (int i = 0; i < network->n; i++) {
      network->adj_list[i]->h = 0;
      network->adj_list[i]->t = 0;
      for (int j = i + 1; j < network->n; j++) {
         edge_insert(network, i, j);
      }
   }
}

// Make new network linearly connected
void make_linear(Network *network, float h, float t) {
   for (int i = 0; i < network->n; i++) {
      if (h == 0 || h*t == 1) {
         network->adj_list[i]->h = 1;
         network->adj_list[i]->t = 1;
      }
      else {
         if (i < network->n / 2) {
            network->adj_list[i]->h = h;
            network->adj_list[i]->t = t;
         }
         else {
            network->adj_list[i]->h = (1.0f - h*t) / (1.0f - t);
            network->adj_list[i]->t = 1.0f - t;
         }
      }
      if (i < network->n - 1) {
         edge_insert(network, i, i + 1);
      }
   }
}

// Make two isolated populations
void make_isolate(Network *network, float h, float t) {
   for (int i = 0; i < network->n / 2; i++) {
      if (h == 0 || h*t == 1) {
         network->adj_list[i]->h = 1;
         network->adj_list[i]->t = 1;
      }
      else {
         network->adj_list[i]->h = h;
         network->adj_list[i]->t = t;
      }
      network->adj_list[i]->namecat = -2;
      for (int j = 0; j < network->n / 2; j++) {
         if (i == j) {
            continue;
         }
         edge_insert(network, i, j);
      }
   }
   for (int i = network->n / 2; i < network->n; i++) {
      if (h == 0 || h*t == 1) {
         network->adj_list[i]->h = 1;
         network->adj_list[i]->t = 1;
      }
      else {
         network->adj_list[i]->h = (1.0f - h*t) / (1.0f - t);
         network->adj_list[i]->t = 1.0f - t;
      }
      network->adj_list[i]->namecat = -3;
      for (int j = network->n / 2; j < network->n; j++) {
         if (i == j) {
            continue;
         }
         edge_insert(network, i, j);
      }
   }
}

// Connect isolated populations
void reconnect(Network *network, float h, float t) {
   for (int i = 0; i < network->n; i++) {
      network->adj_list[i]->h = h;
      network->adj_list[i]->t = t;
      network->adj_list[i]->namecat = -3;
      for (int j = i + 1; j < network->n; j++) {
         edge_insert(network, i, j);
      }
   }
}

// Make new network into a small-world network using the Watts–Strogatz model
void watts_strogatz(Network *network, float h, float t, int K, float beta) {
   for (int i = 0; i < network->n; i++) {
      if (h == 0 || h*t == 1) {
         network->adj_list[i]->h = 1;
         network->adj_list[i]->t = 1;
      }
      else {
         if (i < network->n / 2) {
            network->adj_list[i]->h = h;
            network->adj_list[i]->t = t;
         }
         else {
            network->adj_list[i]->h = (1.0f - h*t) / (1.0f - t);
            network->adj_list[i]->t = 1.0f - t;
         }
      }
      for (int k = 1; k <= K / 2; k++) {
         int j = (i + k) % network->n;
         if (frand() <= beta) {
            j = frand() * network->n;
            while (j == i || contains_edge(network, i, j)) {
               j = (j + 1) % network->n;
            }
         }
         edge_insert(network, i, j);
      }
   }
}

// Helper function for match_agent
float scale(float top, float bottom, float h, float t) {
   if (h == 0 || h*t == 1) {
      return top - bottom;
   }
   if (bottom > t) {
      return ((1.0 - (h*t) / 1.0 - t)) * (top - bottom);
   }
   else if (top > t && bottom <= t) {
      float left = h * (t - bottom);
      float right = ((1.0 - (h*t) / 1.0 - t)) * (top - t);
      return left + right;
   }
   else {
      return h * (top - bottom);
   }
}

// Measure how much of the perceptual space (within a window) the agents agree on
float match_node(Agent *node_x, Agent *node_y, float min, float max, bool env) {
   Category *x = left_most(node_x->tree);
   Category *y = left_most(node_y->tree);

   float acc = 0, bottom = 0, high = 0, low = 0;
   while (x != NULL && y != NULL && bottom <= max) {
      if (x->top < y->top) {
         if (x->top < min) {
            bottom = x->top;
            x = x->next;
         }
         else {
            high = MIN(max, x->top), low = MAX(min, bottom);
            if (x->head != NULL && y->head != NULL && peek(x) == peek(y)) {
               if (env) {
                  float acc_x = scale(high, low, node_x->h, node_x->t);
                  float acc_y = scale(high, low, node_y->h, node_y->t);
                  acc += (acc_x + acc_y) / 2;
               }
               else {
                  acc += high - low;
               }
            }
            bottom = high;
            x = x->next;
         }
      }
      else {
         if (y->top < min) {
            bottom = y->top;
            y = y->next;
         }
         else {
            high = MIN(max, y->top), low = MAX(min, bottom);
            if (x->head != NULL && y->head != NULL && peek(x) == peek(y)) {
               if (env) {
                  float acc_x = scale(high, low, node_x->h, node_x->t);
                  float acc_y = scale(high, low, node_y->h, node_y->t);
                  acc += (acc_x + acc_y) / 2;
               }
               else {
                  acc += high - low;
               }
            }
            bottom = high;
            y = y->next;
         }
      }
   }
   float width_x = scale(max, min, node_x->h, node_x->t);
   float width_y = scale(max, min, node_y->h, node_y->t);
   float width = env ? (width_x + width_y) / 2 : (max - min);
   return acc / width;
}

// Deep clone of the network
Network *clone_network(Network *network) {
   Network *new = create_network(network->n);
   for (int i = 0; i < network->n; i++) {
      new->adj_list[i] = clone_agent(network->adj_list[i]);
   }
   return new;
}

// Deallocate network safely
void delete_network(Network *network) {
   if (network != NULL) {
      for (int i = 0; i < network->n; i++) {
         delete_agent(network->adj_list[i]);
         network->adj_list[i] = NULL;
      }
      free(network);
   }
}