#include <stdint.h>

// based on bsp implementation
extern uint32_t rand_u16();
extern void render(uint32_t x, uint32_t y, uint32_t color);
extern uint32_t get_msec();
extern void sleep_msec(uint32_t msec);

// [l, h)
int32_t rand_in(int32_t l, int32_t h) {
  if (l >= h || h - l >= 0xFFFF) return 0;
  uint32_t range = h - l;
  uint32_t max_valid = 0xFFFF - (0xFFFF % range);
  while (1) {
    uint32_t x = rand_u16();
    if (x < max_valid) return l + (x % range);
  }
}

void shuffle(uint32_t* arr, uint32_t n) {
  for (uint32_t i = 0; i < n - 1; i++) {
    uint32_t j = rand_in(i, n);
    uint32_t temp = arr[i];
    arr[i] = arr[j];
    arr[j] = temp;
  }
}

typedef int32_t fixed; /* 7.9 */

#define fx_from_i32(x) ((x) << 9)
#define fx_to_i32(x) (((x) >> 9) + (((x) & 0x1FF) > 0xFF))
fixed fx_mul(fixed a, fixed b) {
  int32_t p = a * b;
  return (p >> 9) + ((p & 0x1FF) > 0xFF);
}
fixed fx_div(fixed a, fixed b) {
  return ((a << 9) / b) + ((a << 9) % b > b / 2);
}

static uint16_t sqrt_int_lut[64] = {
  0x000, 0x200, 0x2d4, 0x376, 0x400, 0x478, 0x4e6, 0x54a,
  0x5a8, 0x600, 0x653, 0x6a2, 0x6ed, 0x736, 0x77b, 0x7be,
  0x800, 0x83f, 0x87c, 0x8b7, 0x8f1, 0x92a, 0x961, 0x997,
  0x9cc, 0xa00, 0xa32, 0xa64, 0xa95, 0xac5, 0xaf4, 0xb22,
  0xb50, 0xb7d, 0xba9, 0xbd5, 0xc00, 0xc2a, 0xc54, 0xc7d,
  0xca6, 0xcce, 0xcf6, 0xd1d, 0xd44, 0xd6a, 0xd90, 0xdb6,
  0xddb, 0xe00, 0xe24, 0xe48, 0xe6c, 0xe8f, 0xeb2, 0xed5,
  0xef7, 0xf19, 0xf3b, 0xf5c, 0xf7d, 0xf9e, 0xfbf, 0xfdf,
};
static uint16_t sqrt_slope_lut[64] = {
  0x16a, 0x0d1, 0x0a1, 0x088, 0x078, 0x06d, 0x064, 0x05d,
  0x057, 0x053, 0x04f, 0x04b, 0x048, 0x045, 0x043, 0x041,
  0x03f, 0x03d, 0x03b, 0x039, 0x038, 0x037, 0x035, 0x034,
  0x033, 0x032, 0x031, 0x030, 0x02f, 0x02f, 0x02e, 0x02d,
  0x02c, 0x02c, 0x02b, 0x02a, 0x02a, 0x029, 0x029, 0x028,
  0x028, 0x027, 0x027, 0x026, 0x026, 0x025, 0x025, 0x025,
  0x024, 0x024, 0x024, 0x023, 0x023, 0x022, 0x022, 0x022,
  0x022, 0x021, 0x021, 0x021, 0x020, 0x020, 0x020, 0x020,
};
static uint16_t sqrt_slope_frac_lut[16] = {
  0x000, 0x080, 0x0b5, 0x0dd, 0x100, 0x11e, 0x139, 0x152,
  0x16a, 0x180, 0x194, 0x1a8, 0x1bb, 0x1cd, 0x1de, 0x1ef,
};
fixed fx_sqrt(fixed a) {
  if (a <= 0) return 0;
  uint32_t i = (a >> 9) & 0x3F;
  if (i == 0) return sqrt_slope_frac_lut[(a >> 5) & 0xF];
  uint32_t f = a & 0x1FF;
  fixed y = sqrt_int_lut[i];
  fixed slope = sqrt_slope_lut[i];
  return y + ((slope * f) >> 9);
}

typedef struct vec { fixed x, y; } vec;

vec vec_add(vec a, vec b) { return (vec) { .x = a.x + b.x, .y = a.y + b.y }; }
vec vec_sub(vec a, vec b) { return (vec) { .x = a.x - b.x, .y = a.y - b.y }; }
vec vec_mul_i(vec a, fixed x) { return (vec) { .x = fx_mul(a.x, x), .y = fx_mul(a.y, x) }; }
vec vec_div_i(vec a, fixed x) { return (vec) { .x = fx_div(a.x, x), .y = fx_div(a.y, x) }; }
fixed vec_dot(vec a, vec b) { return fx_mul(a.x, b.x) + fx_mul(a.y, b.y); }
fixed vec_len2(vec x) { return vec_dot(x, x); }
fixed vec_len(vec x) { return fx_sqrt(vec_len2(x)); }
fixed vec_dis2(vec a, vec b) { return vec_len2(vec_sub(a, b)); }
fixed vec_dis(vec a, vec b) { return fx_sqrt(vec_dis2(a, b)); }
vec vec_norm(vec a) {
  fixed len = vec_len(a);
  if (len <= 0x1) return (vec) { .x = fx_from_i32(1), .y = 0 };
  return vec_div_i(a, len);
}

#define N 10
#define BORDER_i 25
#define BORDER_fx (fx_from_i32(BORDER_i))
#define g (fx_div(fx_from_i32(98), fx_from_i32(10)))
#define FPS 25
#define STEP_PER_FRAME 2
#define dt (fx_div(fx_from_i32(1), fx_from_i32(FPS * STEP_PER_FRAME)))
#define CANVAS_SIZE 25
#define PGS_ITER_TIMES 5

vec pos[N] = {};
vec v[N] = {};
fixed m[N] = {};
fixed r[N] = {};
fixed e0;
uint32_t colors[N] = {};

uint32_t is_collide[N][N];

void detect_collide() {
  for (uint32_t i = 0; i < N; i++) {
    for (uint32_t j = i + 1; j < N; j++) {
      is_collide[i][j] = is_collide[j][i] 
        = vec_dis2(pos[i], pos[j]) <= fx_mul(r[i] + r[j], r[i] + r[j]);
    }
  }
}

int pgs_once() {
  static uint32_t idxes[N];
  static int idx_inited = 0;
  if (!idx_inited) {
    for (int i = 0; i < N; i++) idxes[i] = i;
    idx_inited = 1;
  }

  shuffle(idxes, sizeof idxes / sizeof idxes[0]);

  int ret = 0;
  for (uint32_t k = 0; k < N; k++) {
    uint32_t i = idxes[k];
    if (pos[i].x - r[i] < 0 && v[i].x < 0) { ret = 1; v[i].x = -v[i].x; }
    if (pos[i].x + r[i] > BORDER_fx && v[i].x > 0) { ret = 1; v[i].x = -v[i].x; }
    if (pos[i].y - r[i] < 0 && v[i].y < 0) { ret = 1; v[i].y = -v[i].y; }
    if (pos[i].y + r[i] > BORDER_fx && v[i].y > 0) { ret = 1; v[i].y = -v[i].y; }

    for (uint32_t p = k + 1; p < N; p++) {
      uint32_t j = idxes[p];
      if (!is_collide[i][j]) continue;
      vec n = vec_sub(pos[j], pos[i]);
      fixed rel_v = vec_dot(v[i], n) - vec_dot(v[j], n);
      if (rel_v <= 0) continue;
      n = vec_norm(n);
      vec vi_n = vec_mul_i(n, vec_dot(v[i], n));
      vec vj_n = vec_mul_i(n, vec_dot(v[j], n));
      v[i] = vec_add(
        vec_sub(v[i], vi_n), vec_add(
          vec_mul_i(vi_n, fx_div(m[i] - m[j], m[i] + m[j])),
          vec_mul_i(vj_n, fx_div(m[j] << 1, m[i] + m[j]))
        )
      );
      v[j] = vec_add(
        vec_sub(v[j], vj_n), vec_add(
          vec_mul_i(vj_n, fx_div(m[j] - m[i], m[i] + m[j])),
          vec_mul_i(vi_n, fx_div(m[i] << 1, m[i] + m[j]))
        )
      );
      ret = 1;
    }
  }
  return ret;
}

void collide() {
  detect_collide();
  for (uint32_t i = 0; i < PGS_ITER_TIMES && pgs_once(); i++);
}

void step() {
  for (uint32_t i = 0; i < N; i++) {
    v[i].x += fx_mul(g, dt);
    pos[i].x += fx_mul(v[i].x, dt);
    pos[i].y += fx_mul(v[i].y, dt);
  }
}

fixed energy_sum() {
  fixed res = 0;
  for (int i = 0; i < N; i++) {
    res += (fx_mul(m[i], vec_len2(v[i])) >> 1) + fx_mul(m[i], fx_mul(g, BORDER_fx - pos[i].x));
  }
  return res;
}

void energy_correction() {
  fixed e = energy_sum();
  fixed s = fx_sqrt(fx_div(e0, e));
  for (uint32_t i = 0; i < N; i++) v[i].x = fx_mul(v[i].x, s);
}

void render_frame() {
  for (uint32_t i = 0; i < CANVAS_SIZE + 2; i++) render(i, 0, 0x0);
  for (uint32_t i = 0; i < CANVAS_SIZE + 2; i++) render(i, CANVAS_SIZE + 1, 0x0);
  for (uint32_t j = 0; j < CANVAS_SIZE + 2; j++) render(0, j, 0x0);
  for (uint32_t j = 0; j < CANVAS_SIZE + 2; j++) render(CANVAS_SIZE + 1, j, 0x0);

  static uint32_t buffer[CANVAS_SIZE][CANVAS_SIZE];
  fixed pixel_size = fx_div(BORDER_fx, fx_from_i32(CANVAS_SIZE));

  for (uint32_t i = 0; i < CANVAS_SIZE; i++)
    for (uint32_t j = 0; j < CANVAS_SIZE; j++)
      buffer[i][j] = 0x00FFFFFF;

  for (uint32_t k = 0; k < N; k++) {
    int32_t min_i = fx_to_i32(fx_div(pos[k].x - r[k], pixel_size));
    int32_t max_i = fx_to_i32(fx_div(pos[k].x + r[k], pixel_size)) + 1;
    int32_t min_j = fx_to_i32(fx_div(pos[k].y - r[k], pixel_size));
    int32_t max_j = fx_to_i32(fx_div(pos[k].y + r[k], pixel_size)) + 1;

    if (min_i < 0) min_i = 0;
    if (max_i > CANVAS_SIZE) max_i = CANVAS_SIZE;
    if (min_j < 0) min_j = 0;
    if (max_j > CANVAS_SIZE) max_j = CANVAS_SIZE;

    for (int32_t i = min_i; i < max_i; i++) {
      for (int32_t j = min_j; j < max_j; j++) {
        vec center = (vec) {
          .x = fx_mul(fx_from_i32(i), pixel_size) + (pixel_size >> 1),
          .y = fx_mul(fx_from_i32(j), pixel_size) + (pixel_size >> 1)
        };

        if (vec_dis2(pos[k], center) <= fx_mul(r[k], r[k])) {
          buffer[i][j] = colors[k];
        }
      }
    }
  }

  for (uint32_t i = 0; i < CANVAS_SIZE; i++) {
    for (uint32_t j = 0; j < CANVAS_SIZE; j++) {
      render(i + 1, j + 1, buffer[i][j]);
    }
  }
}

int main() {
  for (uint32_t i = 0; i < N; i++) {
    m[i] = fx_from_i32(rand_in(1, 7));
    r[i] = fx_sqrt(m[i]);
    pos[i] = (vec) {
      .x = fx_from_i32(rand_in(fx_to_i32(r[i]), BORDER_i - fx_to_i32(r[i]) + 1)),
      .y = fx_from_i32(rand_in(fx_to_i32(r[i]), BORDER_i - fx_to_i32(r[i]) + 1))
    };
    v[i] = (vec) {
      .x = fx_from_i32(rand_in(-5, 6)),
      .y = fx_from_i32(rand_in(-5, 6))
    };
    uint32_t color = rand_in(0, 3);
    colors[i] = color == 0 ? 0x00FF0000 : color == 1 ? 0x0000FF00 : 0x000000FF;
  }
  e0 = energy_sum();

  uint32_t start_msec = get_msec();
  while (1) {
    for (int32_t _ = 0; _ < STEP_PER_FRAME; _++) {
      collide();
      energy_correction();
      step();
    }
    uint32_t end_msec = get_msec();
    int32_t to_sleep = start_msec + 1000 / FPS - end_msec;
    if (to_sleep > 0) sleep_msec(to_sleep);
    render_frame();
    start_msec = get_msec();
  }
}
