import math

Q = 9
SCALE = 1 << Q

sqrt_int = []
sqrt_slope = []

for I in range(64):
  x0 = I
  x1 = I + 1

  y0 = math.sqrt(x0)
  slope = 1 / (2 * math.sqrt((x0 + x1) / 2))

  sqrt_int.append(int(y0 * SCALE))
  sqrt_slope.append(int(slope * SCALE))

print("static uint16_t sqrt_int_lut[64] = {", end='')
for i, v in enumerate(sqrt_int):
  if i % 8 == 0: print('\n  ', end='')
  print(f"0x{v:03x}, ", end='')
print("\n};")

print("static uint16_t sqrt_slope_lut[64] = {", end='')
for i, v in enumerate(sqrt_slope):
  if i % 8 == 0: print('\n  ', end='')
  print(f"0x{v:03x}, ", end='')
print("\n};")

print('static uint16_t sqrt_slope_frac_lut[16] = {', end='')
for I in range(16):
  if I % 8 == 0: print('\n  ', end='')
  print(f'0x{int(math.sqrt(I / 16) * SCALE):03x}, ', end='')
print('\n};')