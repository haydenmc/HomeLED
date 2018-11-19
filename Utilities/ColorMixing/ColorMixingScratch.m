tx = 0.209
ty = 0.294

rx = 0.69306747
ry = 0.30609503

gx = 0.14420122
gy = 0.71912652

bx = 0.13345832
by = 0.05729985

ax = 0.4872689
ay = 0.4060504

wx = 0.31253751
wy = 0.33643754

A = [
(rx - tx)/ry, (gx - tx)/gy, (bx - tx)/by, (ax - tx)/ay, (wx - tx)/wy;
(ry - ty)/ry, (gy - ty)/gy, (by - ty)/by, (ay - ty)/ay, (wy - ty)/wy;
1, 1, 1, 1, 1
]

Ainv = A' * (A * (A'))^(-1)

Ainv * [0; 0; 1]