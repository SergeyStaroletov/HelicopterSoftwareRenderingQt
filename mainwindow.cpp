#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QPainter>
#include <QMessageBox>
#include <chrono>
#include <cmath>

#define MAXX 800
#define MAXY 600

typedef struct {
    float x;
    float y;
} TPointf;
typedef struct {
    float x;
    float y;
    float z;
} TPoint3f;
typedef struct {
    int x;
    int y;
    int z;
} TPoint3;
typedef struct {
    int t1;
    int t2;
    int t3;
} TTreug;
typedef struct {
    int R;
    int G;
    int B;
} TObjColor;

struct center {
    float x, y;
} center;

//watcher
float p; // 2110 900 4025
float q;
float r;
std::string fname;

const int max_o = 10;
const int max_p = 1000;
const int max_c = 100;
const int max_ist = 3;

TPoint3 tp[max_c];

float zbuffer[MAXX][MAXY];

long framebuffer[MAXX][MAXY];
TPoint3f normals[max_o][max_p];
float ix[max_ist];
float iy[max_ist];
float iz[max_ist];

float ii;
const float dd = 0.1;

int n_p[max_o];
int n_c;
int n_o;
int maxx;
int maxy;

TObjColor colors[max_o];
const float pi = 3.1415926;
bool anim = false;
int tics = 0;
int frames = 0;

TPointf coords[max_o][max_p];
TPoint3f coords3[max_o][max_p];
TTreug conn[max_o][max_p];
int n_connect[max_o];

uchar *ptr;
float K1, K2;

void conv_3d(float x, float y, float z, float &xe, float &ye) {
    float t = -r / (z - r);
    xe = p + t * (x - p);
    ye = q + t * (y - q);
}

bool read_file() {
    float x, y, z;
    float xe, ye;
    FILE *f = fopen(fname.c_str(), "r");
    if (!f) {
        return false;
    }
    fscanf(f, "%d", &n_o);
    for (int o = 0; o < n_o; o++) {
        fscanf(f, "%d %d %d", &colors[o].R, &colors[o].G, &colors[o].B);
        fscanf(f, "%d", &n_p[o]);
        n_p[o] = n_p[o];
        for (int i = 0; i < n_p[o]; i++) {
            fscanf(f, "%f %f %f", &x, &y, &z);
            conv_3d(x, y, z, coords[o][i].x, coords[o][i].y);
            coords3[o][i].x = x;
            coords3[o][i].y = y;
            coords3[o][i].z = z;
        }
        fscanf(f, "%d", &n_connect[o]);
        for (int i = 0; i < n_connect[o]; i++) {
            fscanf(f, "%d %d %d", &conn[o][i].t1, &conn[o][i].t2, &conn[o][i].t3);
        }
    }
    fclose(f);
    return true;
}

void prepare_array() {
    for (int o = 0; o < n_o; o++)
        for (int i = 0; i < n_p[o]; i++) {
            float x = coords3[o][i].x;
            float y = coords3[o][i].y;
            float z = coords3[o][i].z;
            conv_3d(x, y, z, coords[o][i].x, coords[o][i].y);
        }
}

void getcm(int id, float &cmx, float &cmy, float &cmz) {
    for (int i = 0; i < n_connect[id]; i++) {

        cmx += (coords3[id][conn[id][i].t1].x + coords3[id][conn[id][i].t2].x +
                coords3[id][conn[id][i].t3].x) /
               3;
        cmy += (coords3[id][conn[id][i].t1].y + coords3[id][conn[id][i].t2].y +
                coords3[id][conn[id][i].t3].y) /
               3;
        cmz += (coords3[id][conn[id][i].t1].z + coords3[id][conn[id][i].t2].z +
                coords3[id][conn[id][i].t3].z) /
               3;
    }

    cmx = cmx / (n_connect[id]);
    cmy = cmy / (n_connect[id]);
    cmz = cmz / (n_connect[id]);
}

void minuscm(int id, float cmx, float cmy, float cmz) {
    for (int i = 0; i < n_p[id]; i++) {
        coords3[id][i].x -= cmx;
        coords3[id][i].y -= cmy;
        coords3[id][i].z -= cmz;
    }
}

void pluscm(int id, float cmx, float cmy, float cmz) {
    for (int i = 0; i < n_p[id]; i++) {
        coords3[id][i].x += cmx;
        coords3[id][i].y += cmy;
        coords3[id][i].z += cmz;
    }
}

// cos sin
//-sin  cos
void rotate_z(float fi, float &x, float &y, float &z) {
    float xn = x * cos(fi) - y * sin(fi);
    float yn = x * sin(fi) + y * cos(fi);
    x = xn;
    y = yn;
}

void rotate_x(float fi, float &x, float &y, float &z) {
    float yn = y * cos(fi) - z * sin(fi);
    float zn = y * sin(fi) + z * cos(fi);
    y = yn;
    z = zn;
}

void rotate_y(float fi, float &x, float &y, float &z) {
    float xn = x * cos(fi) - z * sin(fi);
    float zn = x * sin(fi) + z * cos(fi);
    x = xn;
    z = zn;
}

void zoom_plus(float k) {
    for (int o = 0; o < n_o; o++)
        for (int i = 0; i < n_p[o]; i++) {
            coords3[o][i].x *= k;
            coords3[o][i].y *= k;
            coords3[o][i].z *= k;
        }
}

void zoom_minus(float k) {
    for (int o = 0; o < n_o; o++)
        for (int i = 0; i < n_p[o]; i++) {
            coords3[o][i].x /= k;
            coords3[o][i].y /= k;
            coords3[o][i].z /= k;
        }
}

void rotate_array_z(float fi, int id) {
    float cmx = 0, cmy = 0, cmz = 0;
    float COS, SIN;
    float xn, yn;
    COS = cos(fi);
    SIN = sin(fi);
    if (id != -1) {
        getcm(id, cmx, cmy, cmz);

        minuscm(id, cmx, cmy, cmz);
        for (int i = 0; i < n_p[id]; i++) {
            xn = coords3[id][i].x * COS - coords3[id][i].y * SIN;
            coords3[id][i].y = coords3[id][i].x * SIN + coords3[id][i].y * COS;
            coords3[id][i].x = xn;
        }
        if (id != -1)
            pluscm(id, cmx, cmy, cmz);
    } else
        for (int o = 0; o < n_o; o++)
            for (int i = 0; i < n_p[o]; i++)
                rotate_z(fi, coords3[o][i].x, coords3[o][i].y, coords3[o][i].z);
}

void rotate_array_x(float fi, int id) {
    float cmx = 0, cmy = 0, cmz = 0;
    float COS, SIN, yn;
    COS = cos(fi);
    SIN = sin(fi);
    if (id != -1) {
        getcm(id, cmx, cmy, cmz);
        minuscm(id, cmx, cmy, cmz);
        for (int i = 0; i < n_p[id]; i++) {
            yn = coords3[id][i].y * COS - coords3[id][i].z * SIN;
            coords3[id][i].z = coords3[id][i].y * SIN + coords3[id][i].z * COS;
            coords3[id][i].y = yn;
        }
        pluscm(id, cmx, cmy, cmz);
    } else
        for (int o = 0; o < n_o; o++)
            for (int i = 0; i < n_p[o]; i++)
                rotate_x(fi, coords3[o][i].x, coords3[o][i].y, coords3[o][i].z);
}

void rotate_array_y(float fi, int id) {
    float cmx = 0, cmy = 0, cmz = 0;
    float COS, SIN, xn;
    COS = cos(fi);
    SIN = sin(fi);
    if (id != -1) {
        getcm(id, cmx, cmy, cmz);
        minuscm(id, cmx, cmy, cmz);
        for (int i = 0; i < n_p[id]; i++) {
            xn = coords3[id][i].x * COS - coords3[id][i].z * SIN;
            coords3[id][i].z = coords3[id][i].x * SIN + coords3[id][i].z * COS;
            coords3[id][i].x = xn;
        }
        pluscm(id, cmx, cmy, cmz);
    } else
        for (int o = 0; o < n_o; o++)
            for (int i = 0; i < n_p[o]; i++)
                rotate_y(fi, coords3[o][i].x, coords3[o][i].y, coords3[o][i].z);
}

void move_array_x(float d, int id) {
    if (id != -1) {
        for (int i = 0; i < n_p[id]; i++)
            coords3[id][i].x += d;
    } else
        for (int o = 0; o < n_o; o++)
            move_array_x(d, o);
}

void move_array_y(float d, int id) {
    if (id != -1) {
        for (int i = 0; i < n_p[id]; i++)
            coords3[id][i].y += d;
    } else
        for (int o = 0; o < n_o; o++)
            move_array_y(d, o);
}

void move_array_z(float d, int id) {
    if (id != -1) {
        for (int i = 0; i < n_p[id]; i++)
            coords3[id][i].z += d;
    } else
        for (int o = 0; o < n_o; o++)
            move_array_z(d, o);
}

void MainWindow::redraw() {

    bool back_light = false;
    int ind = 0;
    int saturation = 20;

    prepare_array();

    float max_u_ist[max_ist], min_u_ist[max_ist];

    max_u_ist[0] = -32767.0 * (32767.0), min_u_ist[0] = +32767.0 * (32767.0);

    for (int o = 0; o < n_o; o++) {
        for (int i = 0; i < n_p[o]; i++) {
            float d1 =
                (((*coords3 + ind + i)->x - *ix) * ((*coords3 + ind + i)->x - *ix) +
                 ((*coords3 + ind + i)->y - *iy) * ((*coords3 + ind + i)->y - *iy) +
                 ((*coords3 + ind + i)->z - *iz) * ((*coords3 + ind + i)->z - *iz));
            if (d1 > *max_u_ist)
                *max_u_ist = d1;
            if (d1 < *min_u_ist)
                *min_u_ist = d1;
        }
        ind += max_p;
    }

    ind = 0;

    for (int zb1 = 0; zb1 < MAXX; zb1++) {
        for (int zb2 = 0; zb2 < MAXY; zb2++)
            *(*zbuffer + ind + zb2) = 32768.0;
        ind += MAXY;
    }

    maxx = MAXX;
    maxy = MAXY;

    K1 = 80;
    K2 = K1 * maxy / maxx;

    image.fill(Qt::black);

    for (int o = 0; o < n_o; o++)
        for (int i = 0; i < n_connect[o]; i++) {
            float x1, x2, y1, y2, z1, z2, A, B, C, D;
            tp[0].x = center.x + (coords[o][conn[o][i].t1].x) * K1;
            tp[1].x = center.x + (coords[o][conn[o][i].t2].x) * K1;
            tp[2].x = center.x + (coords[o][conn[o][i].t3].x) * K1;

            tp[3].x = center.x + (coords[o][conn[o][i].t1].x) * K1;
            tp[3].y = center.y - (coords[o][conn[o][i].t1].y) * K2;

            tp[0].y = center.y - (coords[o][conn[o][i].t1].y) * K2;
            tp[1].y = center.y - (coords[o][conn[o][i].t2].y) * K2;
            tp[2].y = center.y - (coords[o][conn[o][i].t3].y) * K2;

            x1 = coords3[o][conn[o][i].t1].x - coords3[o][conn[o][i].t3].x;
            y1 = coords3[o][conn[o][i].t1].y - coords3[o][conn[o][i].t3].y;
            z1 = coords3[o][conn[o][i].t1].z - coords3[o][conn[o][i].t3].z;
            x2 = coords3[o][conn[o][i].t2].x - coords3[o][conn[o][i].t3].x;

            y2 = coords3[o][conn[o][i].t2].y - coords3[o][conn[o][i].t3].y;
            z2 = coords3[o][conn[o][i].t2].z - coords3[o][conn[o][i].t3].z;
            A = (y1 * z2) - (z1 * y2);
            B = -((x1 * z2) - (z1 * x2));
            C = (x1 * y2) - (y1 * x2);
            D = -A * coords3[o][conn[o][i].t2].x - B * coords3[o][conn[o][i].t2].y -
                C * coords3[o][conn[o][i].t2].z;
            float Nx = A;
            float Ny = B;
            float Nz = C;

            TPoint3f AA, BB, CC;
            int nmax = 0;
            int nmin = 0;
            float min, maX;
            min = tp[0].y;
            if (tp[1].y < min) {
                min = tp[1].y;
                nmin = 1;
            }
            if (tp[2].y < min) {
                nmin = 2;
            }

            maX = tp[0].y;
            if (tp[1].y > maX) {
                maX = tp[1].y;
                nmax = 1;
            }
            if (tp[2].y > maX) {
                nmax = 2;
            }
            CC.x = tp[nmax].x;
            CC.y = tp[nmax].y;
            if (nmax == 0)
                CC.z = coords3[o][conn[o][i].t1].z;
            else if (nmax == 1)
                CC.z = coords3[o][conn[o][i].t2].z;
            else if (nmax == 2)
                CC.z = coords3[o][conn[o][i].t3].z;

            AA.x = tp[nmin].x;
            AA.y = tp[nmin].y;

            if (nmin == 0)
                AA.z = coords3[o][conn[o][i].t1].z;
            else if (nmin == 1)
                AA.z = coords3[o][conn[o][i].t2].z;
            else if (nmin == 2)
                AA.z = coords3[o][conn[o][i].t3].z;

            int sr = 0;
            if (sr == nmax)
                sr++;
            if (sr == nmin)
                sr++;
            if (sr == nmax && sr != 2)
                sr++;

            BB.x = tp[sr].x;
            BB.y = tp[sr].y;

            if (sr == 0)
                BB.z = coords3[o][conn[o][i].t1].z;
            else if (sr == 1)
                BB.z = coords3[o][conn[o][i].t2].z;
            else if (sr == 2)
                BB.z = coords3[o][conn[o][i].t3].z;

            // Scanline
            for (int sy = AA.y; sy <= CC.y; sy++) {
                int X1 = 0, X2 = 0;

                if (sy >= 0 && sy <= maxy) {
                    if (CC.y != AA.y) {
                        X1 = AA.x + (sy - AA.y) * (CC.x - AA.x) / (CC.y - AA.y);
                        if (sy < BB.y)
                            X2 = AA.x + (sy - AA.y) * (BB.x - AA.x) / (BB.y - AA.y);
                        else {
                            if (CC.y == BB.y)
                                X2 = BB.x;
                            else
                                X2 = BB.x + (sy - BB.y) * (CC.x - BB.x) / (CC.y - BB.y);
                        }
                        if (X1 > X2) {
                            int tmp = X1;
                            X1 = X2;
                            X2 = tmp;
                        }

                        if (X2 > MAXX) {
                            continue;
                        }

                        ptr = image.scanLine(sy);
                        long double dz = 0;
                        bool p_fl = true;

                        for (int h = X1 + 1; h <= X2; h++) {

                            float xe, ye;

                            xe = (h - center.x) / K1;
                            ye = (center.y - sy) / K2;

                            dz = (A * xe + B * ye + D) /
                                 ((A * (xe - p) / r + B * (ye - q) / r - C));

                            if (h <= maxx && h >= 0 && sy <= maxy && sy >= 0)
                                if (zbuffer[h][sy] == 32768.0 || zbuffer[h][sy] < dz - r) {
                                    float d1, d2;

                                    d1 = ((xe - ix[0]) * (xe - ix[0]) +
                                          (ye - iy[0]) * (ye - iy[0]) +
                                          (dz - iz[0]) * (dz - iz[0]));

                                    float g1 =
                                        (d1 - min_u_ist[0]) / (max_u_ist[0] - min_u_ist[0]);

                                    if (g1 > 1 || g1 < 0) {
                                        if (g1 > 1)
                                            g1 = 1;
                                        if (g1 < 0)
                                            g1 = 0;
                                    }

                                    g1 = g1 * g1;
                                    g1 = 1 - g1;

                                    float sum_x = 0, sum_y = 0, sum_z = 0;

                                    sum_x = (coords3[o][conn[o][i].t1].x +
                                             coords3[o][conn[o][i].t2].x +
                                             coords3[o][conn[o][i].t3].x) /
                                            3;
                                    sum_y = (coords3[o][conn[o][i].t1].y +
                                             coords3[o][conn[o][i].t2].y +
                                             coords3[o][conn[o][i].t3].y) /
                                            3;
                                    sum_z = (coords3[o][conn[o][i].t1].z +
                                             coords3[o][conn[o][i].t2].z +
                                             coords3[o][conn[o][i].t3].z) /
                                            3;

                                    sum_x -= xe;
                                    sum_y -= ye;
                                    sum_z -= dz;

                                    float v1 = p - sum_x;
                                    float v2 = q - sum_y;
                                    float v3 = r - sum_z;

                                    float norm_x = Nx;
                                    float norm_y = Ny;
                                    float norm_z = Nz;

                                    float c = (v1 * norm_x + v2 * norm_y + v3 * norm_z) /
                                              (sqrt(v1 * v1 + v2 * v2 + v3 * v3) *
                                               sqrt(norm_x * norm_x + norm_y * norm_y +
                                                    norm_z * norm_z));
                                    c = fabs(c);

                                    g1 = c * g1;

                                    g1 = g1 * (1.0 * saturation / 5.0);

                                    if (g1 > 1)
                                        g1 = 1;
                                    if (back_light && g1 < 0.2)
                                        g1 = 0.2;

                                    ptr[h * 4] = colors[o].B * g1;
                                    ptr[h * 4 + 1] = colors[o].G * g1;
                                    ptr[h * 4 + 2] = colors[o].R * g1;

                                    zbuffer[h][sy] = dz - r;
                                }
                        }
                    }
                }
            }
        }
    ui->label->setPixmap(QPixmap::fromImage(image));
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    image = QImage(MAXX + 1, MAXY + 1, QImage::Format_ARGB32);
    image.fill(Qt::black);
    ui->label->setPixmap(QPixmap::fromImage(image));
    connect(&timer, &QTimer::timeout, this, &MainWindow::checkFPS);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::checkFPS() {
    double fpspersec = frames / 5.0;
    frames = 0;
    ui->labelFPS->setText("FPS: " + QString::number(fpspersec, 'f', 1));
}

void MainWindow::on_pushButtonStart_clicked() {

    if (anim) {
        anim = false;
        timer.stop();
        ui->pushButtonStart->setText("Start");
        return;
    }

    fname = "heli_nfs3.txt";

    n_o = 1;
    p = 211; // 2110 900 4025
    q = 90;
    r = 402.5;
    ix[0] = 0;
    iy[0] = 0;
    iz[0] = 4.8;
    ix[1] = 0;
    iy[1] = 0;
    iz[1] = -2.8;
    ii = 0.2;
    center.x = MAXX / 2;
    center.y = MAXY / 2;

    if (!read_file()) {
        QMessageBox::about(this, "Model is not found", "Check for heli_nfs3.txt!");
        return;
    }
    p = -169;
    q = 50;
    r = 412.5;
    tics = 0;
    frames = 0;
    anim = true;
    ui->pushButtonStart->setText("Stop");

    float aa = 1.34567;
    int ap = 0;

    int ab = 0;
    int pov = 0;
    int pov1 = 0;
    int pov2 = 0;

    int press1 = 0;
    float press = aa;
    int press11 = 0;
    float press_1 = aa;
    int press21 = 0;
    float press_2 = aa;

    std::chrono::seconds sec(5);
    timer.setInterval(sec);
    timer.start();

    while (anim) {

        // reset moving
        if ((press_2 == 0) && !ui->calmCheck->isChecked()) {
            aa = 1.34567;
            ab = 0;
            pov = 0;
            pov1 = 0;
            pov2 = 0;
            press1 = 0;
            press = aa;
            press11 = 0;
            press_1 = aa;
            press21 = 0;
            press_2 = aa;
            p = -169;
            q = 50;
            r = 412.5;
        }

        QCoreApplication::processEvents();
        redraw();

        rotate_array_x(-pi / 8, 2);
        rotate_array_y(-pi / 16, 1);
        ab += aa;
        if (ab > 200 || ab <= -200) {
            aa = -1 * aa;
        }
        p += ((aa > 0) - (aa < 0)) * 2;
        if (p >= 100 || aa == 0) {
            aa = 0;
            pov++;
            if (pov == 20) {
                pov = 19;
                q -= press;
                press1++;
                if (press1 == 7 * 20) {
                    press = 0;
                    press1--;
                    pov1++;
                    if (pov1 == 20) {
                        pov1 = 19;
                        p -= press_1;
                        press11++;
                        if (press11 == 10 * 20) {
                            press_1 = 0;
                            press11--;
                            pov2++;
                            if (pov2 == 20) {
                                pov2 = 19;
                                q += press_2;
                                press21++;
                                if (press21 == 8 * 20) {
                                    press_2 = 0;
                                }
                            }
                        }
                    }
                }
            }
        }
        frames++;
    }
}
