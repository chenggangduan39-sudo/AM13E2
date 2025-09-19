#include "qtk/sci/optimize/qtk_optimize.h"
#include "wtk/core/wtk_alloc.h"
#include "qtk/core/qtk_min_heap.h"
#include "qtk/core/qtk_type.h"
#include "qtk/math/qtk_math.h"

static int augmenting_path_(int nr, int nc, float *cost, float *u, float *v,
                            int *path, int *row4col, float *shortest_path_costs,
                            int cur_row, char *SR, char *SC, int *remaining,
                            float *min_val) {
    float minVal = 0.0;
    int sink = -1;
    int num_remaining = nc;

    for (int i = 0; i < nc; i++) {
        remaining[i] = nc - i - 1;
    }

    memset(SR, 0, sizeof(SR[0]) * nr); // default false
    memset(SC, 0, sizeof(SC[0]) * nc); // default false
    for (int i = 0; i < nc; i++) {
        shortest_path_costs[i] = INFINITY;
    }

    while (sink == -1) {
        int index = -1;
        float lowest = INFINITY;
        SR[cur_row] = 1;

        for (int i = 0; i < num_remaining; i++) {
            int j = remaining[i];
            float r = minVal + cost[cur_row * nc + j] - u[cur_row] - v[j];
            if (r < shortest_path_costs[j]) {
                path[j] = cur_row;
                shortest_path_costs[j] = r;
            }
            if (shortest_path_costs[j] < lowest ||
                (shortest_path_costs[j] == lowest && row4col[j] == -1)) {
                lowest = shortest_path_costs[j];
                index = i;
            }
        }

        minVal = lowest;
        if (minVal == INFINITY) { // infeasible cost matrix
            goto fail;
        }

        int j = remaining[index];
        if (row4col[j] == -1) {
            sink = j;
        } else {
            cur_row = row4col[j];
        }

        SC[j] = 1;
        remaining[index] = remaining[--num_remaining];
    }

    *min_val = minVal;
    return sink;
fail:
    return -1;
}

typedef struct {
    int v;
    int idx;
} sort_item_;

static int item_cmp_(sort_item_ *e1, sort_item_ *e2) {
    if (e1->v > e2->v) {
        return 1;
    }
    if (e1->v < e2->v) {
        return -1;
    }
    return 0;
}

int qtk_optimize_linear_sum_assignment(int r, int c, float *cost, int *a,
                                       int *b) {
    float *cost_trans = NULL;
    float *u, *v, *shortest_path_costs;
    int *path, *col4row, *row4col;
    char *SR, *SC;
    int *remaining;

    if (c < r) {
        int tmp;
        cost_trans = wtk_malloc(sizeof(float) * r * c);
        for (int i = 0; i < c; i++) {
            for (int j = 0; j < r; j++) {
                cost_trans[i * r + j] = cost[j * c + i];
            }
        }
        cost = cost_trans;
        tmp = r;
        r = c;
        c = tmp;
    }

    u = wtk_calloc(sizeof(u[0]), r);
    v = wtk_calloc(sizeof(v[0]), c);
    shortest_path_costs = wtk_malloc(sizeof(shortest_path_costs[0]) * c);
    path = wtk_malloc(sizeof(path[0]) * c);
    col4row = wtk_malloc(sizeof(col4row[0]) * r);
    row4col = wtk_malloc(sizeof(row4col[0]) * c);
    SR = wtk_malloc(sizeof(SR[0]) * r);
    SC = wtk_malloc(sizeof(SC[0]) * c);
    remaining = wtk_malloc(sizeof(remaining[0]) * c);

    for (int i = 0; i < c; i++) {
        path[i] = -1;
        row4col[i] = -1;
    }

    for (int i = 0; i < r; i++) {
        col4row[i] = -1;
    }

    for (int cur_row = 0; cur_row < r; cur_row++) {
        float min_val;
        int sink = augmenting_path_(r, c, cost, u, v, path, row4col,
                                    shortest_path_costs, cur_row, SR, SC,
                                    remaining, &min_val);
        if (sink < 0) {
            goto fail;
        }

        u[cur_row] += min_val;
        for (int i = 0; i < r; i++) {
            if (SR[i] && i != cur_row) {
                u[i] += min_val - shortest_path_costs[col4row[i]];
            }
        }

        for (int j = 0; j < c; j++) {
            if (SC[j]) {
                v[j] -= min_val - shortest_path_costs[j];
            }
        }

        int j = sink;
        while (1) {
            int i = path[j];
            int tmp;
            row4col[j] = i;

            tmp = col4row[i];
            col4row[i] = j;
            j = tmp;

            if (i == cur_row) {
                break;
            }
        }
    }

    if (cost_trans) {
        qtk_min_heap_t min_heap;
        sort_item_ item;
        qtk_min_heap_init2(&min_heap, sizeof(sort_item_), r,
                           cast(qtk_min_heap_cmp_f, item_cmp_));
        for (int i = 0; i < r; i++) {
            item.v = col4row[i];
            item.idx = i;
            qtk_min_heap_push(&min_heap, &item);
        }
        for (int i = 0; i < r; i++) {
            qtk_min_heap_pop(&min_heap, &item);
            a[i] = item.v;
            b[i] = item.idx;
        }
        qtk_min_heap_clean(&min_heap);
        wtk_free(cost_trans);
    } else {
        for (int i = 0; i < r; i++) {
            a[i] = i;
            b[i] = col4row[i];
        }
    }
    wtk_free(u);
    wtk_free(v);
    wtk_free(shortest_path_costs);
    wtk_free(path);
    wtk_free(col4row);
    wtk_free(row4col);
    wtk_free(SR);
    wtk_free(SC);
    wtk_free(remaining);
    return 0;
fail:
    return -1;
}
