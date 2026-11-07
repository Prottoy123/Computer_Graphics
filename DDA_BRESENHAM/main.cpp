// scene.cpp
// GLUT scene recreating the provided image with DDA/Bresenham/Midpoint circle,
// 2D transforms, keyboard-controlled walking astronaut, rotating ring.
// Compile: g++ -O2 scene.cpp -o scene -lGL -lGLU -lglut

#include <GL/glut.h>
#include <cmath>
#include <vector>
#include <iostream>

const int WIN_W = 1536;
const int WIN_H = 1024;

// Animation state
float ringAngle = 0.0f;
float ringSpeed = 0.45f; // deg per frame
int astronautX = 535;     // starting center-x of astronaut bbox
int astronautY = 550;     // base y (feet baseline)
bool walking = true;
int walkFrame = 0;

// Colors convenience
struct Color { float r,g,b; };
Color skyTop = {0.06f,0.06f,0.2f};   // deep night
Color skyBottom = {0.04f,0.12f,0.35f};
Color moonColor = {0.2f,0.28f,0.45f};
Color planetColor = {0.08f,0.13f,0.3f};
Color groundColor = {0.42f,0.1f,0.20f};
Color mountainColor = {0.08f,0.08f,0.18f};
Color astronautWhite = {0.92f,0.92f,0.98f};
Color visorBlack = {0.03f,0.03f,0.06f};
Color mushroomCap = {0.45f,0.15f,0.46f};
Color stemColor = {0.08f,0.45f,0.36f};
Color rockColor = {0.25f,0.09f,0.14f};

// -------------------- Basic drawing helpers --------------------
void setColor(const Color &c) { glColor3f(c.r,c.g,c.b); }
void drawFilledPoly(const std::vector<std::pair<int,int>> &pts) {
    glBegin(GL_POLYGON);
    for(auto &p: pts) glVertex2i(p.first, p.second);
    glEnd();
}
void drawPolyline(const std::vector<std::pair<int,int>> &pts) {
    glBegin(GL_LINE_STRIP);
    for(auto &p: pts) glVertex2i(p.first, p.second);
    glEnd();
}

// -------------------- Graphics algorithms --------------------
// DDA line algorithm (draws points)
void drawLineDDA(int x0,int y0,int x1,int y1) {
    int dx = x1 - x0, dy = y1 - y0;
    int steps = std::max(abs(dx), abs(dy));
    if (steps==0) { glBegin(GL_POINTS); glVertex2i(x0,y0); glEnd(); return; }
    float xInc = dx / (float)steps;
    float yInc = dy / (float)steps;
    float x = x0, y = y0;
    glBegin(GL_POINTS);
    for(int i=0;i<=steps;i++){
        glVertex2i((int)round(x),(int)round(y));
        x += xInc; y += yInc;
    }
    glEnd();
}

// Bresenham line algorithm (draws points)
void drawLineBresenham(int x0,int y0,int x1,int y1) {
    bool steep = abs(y1 - y0) > abs(x1 - x0);
    if(steep) { std::swap(x0,y0); std::swap(x1,y1); }
    if (x0 > x1) { std::swap(x0,x1); std::swap(y0,y1); }
    int dx = x1 - x0, dy = abs(y1 - y0);
    int err = dx/2;
    int ystep = (y0 < y1) ? 1 : -1;
    int y = y0;
    glBegin(GL_POINTS);
    for(int x = x0; x <= x1; ++x) {
        if(steep) glVertex2i(y, x); else glVertex2i(x, y);
        err -= dy;
        if(err < 0) { y += ystep; err += dx; }
    }
    glEnd();
}

// Midpoint Circle (draws points)
void drawMidpointCircle(int xc,int yc,int r) {
    int x = 0;
    int y = r;
    int d = 1 - r;
    glBegin(GL_POINTS);
    while(x <= y) {
        glVertex2i(xc + x, yc + y);
        glVertex2i(xc - x, yc + y);
        glVertex2i(xc + x, yc - y);
        glVertex2i(xc - x, yc - y);
        glVertex2i(xc + y, yc + x);
        glVertex2i(xc - y, yc + x);
        glVertex2i(xc + y, yc - x);
        glVertex2i(xc - y, yc - x);
        if(d < 0) d += 2*x + 3;
        else { d += 2*(x - y) + 5; y--; }
        x++;
    }
    glEnd();
}

// Utility to draw filled circle by drawing concentric midpoint circle rings
void drawFilledCircle(int xc,int yc,int r) {
    for(int rr = r; rr >= 0; --rr) {
        drawMidpointCircle(xc,yc,rr);
    }
}

// -------------------- Scene geometry (approx from the image) --------------------
// Mountains silhouettes: multiple polygons carefully chosen to match the image silhouette
std::vector<std::pair<int,int>> mountainLeft = {
    {0,650},{60,600},{140,560},{230,560},{320,620},{420,580},{600,600},{750,650},{0,650}
};
std::vector<std::pair<int,int>> mountainRight = {
    {750,650},{850,600},{960,560},{1100,570},{1220,620},{1350,600},{1536,650},{750,650}
};

// Ground contour (wavy)
std::vector<std::pair<int,int>> groundShape = {
    {0,760},{80,740},{180,760},{300,730},{420,750},{540,740},{660,760},{820,740},
    {920,760},{1050,730},{1200,750},{1320,740},{1536,760},{1536,1024},{0,1024}
};

// Rocks positions (to draw ellipses/polygons)
std::vector<std::pair<int,int>> rocks = {
    {140,880},{330,870},{980,880},{1400,880}
};

// Mushrooms definitions (stem center and cap)
struct Mushroom { int cx, cy; int stemH, capW, capH; };
Mushroom mLeft = {190,620,90,180,60};
Mushroom mRightBig = {1250,580,200,240,120};
Mushroom mRightMed = {1075,640,140,140,80};
Mushroom mRightSmall = {1330,660,110,110,60};

// Astronaut relative geometry (we will translate by astronautX, astronautY)
struct AstrGeom {
    // derived sizes
    int bodyW = 130, bodyH = 220;
    int headR = 45; // radius for visor circle center offset
    int backpackW = 60, backpackH = 120;
};
AstrGeom AG;

// Flashlight cone polygon relative offsets
std::vector<std::pair<int,int>> flashlightCone = {
    {70,45}, {210,130}, {380,190}, {380,210}, {210,160}, {70,85}
};

// -------------------- Drawing functions for each element --------------------
void drawGradientSky() {
    glBegin(GL_QUADS);
    glColor3f(skyTop.r, skyTop.g, skyTop.b); glVertex2i(0,0);
    glColor3f(skyTop.r, skyTop.g, skyTop.b); glVertex2i(WIN_W,0);
    glColor3f(skyBottom.r, skyBottom.g, skyBottom.b); glVertex2i(WIN_W, WIN_H/2);
    glColor3f(skyBottom.r, skyBottom.g, skyBottom.b); glVertex2i(0, WIN_H/2);
    glEnd();
    glColor3f(1,1,1);
    // stars (some points)
    glPointSize(2.0f);
    glBegin(GL_POINTS);
    for(int i=0;i<70;i++){
        int sx = (i*37) % WIN_W;
        int sy = 40 + ((i*83) % (WIN_H/2));
        glVertex2i(sx, sy);
    }
    glEnd();
}

void drawMoon() {
    // use mid-point circle filled
    setColor(moonColor);
    drawFilledCircle(260,200,55);
    // small craters as darker circles
    setColor({0.16f,0.22f,0.34f});
    drawFilledCircle(230,180,12);
    drawFilledCircle(285,220,10);
}

void drawPlanetAndRing() {
    // planet
    setColor(planetColor);
    drawFilledCircle(1120,220,140);
    // planet subtle stripes: arcs using circle draws scaled vertically
    glPushMatrix();
    glTranslatef(1120,220,0);
    // draw ring by generating an ellipse polygon rotated by ringAngle
    glPopMatrix();

    // draw ring (an ellipse) but rotate only the ring
    float a = 280.0f; // semi-major
    float b = 60.0f;  // semi-minor
    int segments = 120;
    setColor({0.07f,0.08f,0.16f});
    glPushMatrix();
    glTranslatef(1120,220,0);
    glRotatef(ringAngle, 0.0f,0.0f,1.0f); // rotate ONLY the ring
    glBegin(GL_POLYGON);
    for(int i=0;i<segments;i++){
        float theta = (2.0f * M_PI * i) / segments;
        float x = a * cos(theta);
        float y = b * sin(theta);
        glVertex2f(x, y);
    }
    glEnd();
    // inner hole to make the ring shape
    setColor(planetColor);
    glBegin(GL_POLYGON);
    float a2 = 200.0f, b2 = 45.0f;
    for(int i=0;i<segments;i++){
        float theta = (2.0f * M_PI * i) / segments;
        float x = a2 * cos(theta);
        float y = b2 * sin(theta);
        glVertex2f(x,y);
    }
    glEnd();
    glPopMatrix();
}

void drawMountains() {
    setColor(mountainColor);
    drawFilledPoly(mountainLeft);
    drawFilledPoly(mountainRight);

    // Use Bresenham to draw silhouette outlines for crisp edge (require line points)
    setColor({0.03f,0.03f,0.06f});
    for (size_t i=1;i<mountainLeft.size();++i)
        drawLineBresenham(mountainLeft[i-1].first,mountainLeft[i-1].second,
                          mountainLeft[i].first,mountainLeft[i].second);
    for (size_t i=1;i<mountainRight.size();++i)
        drawLineBresenham(mountainRight[i-1].first,mountainRight[i-1].second,
                          mountainRight[i].first,mountainRight[i].second);
}

void drawGroundAndRocks() {
    setColor(groundColor);
    drawFilledPoly(groundShape);

    // rocks — draw as ellipses using scaled midpoint circles
    setColor(rockColor);
    for(auto &r : rocks) {
        int cx = r.first, cy = r.second;
        // draw a filled ellipse by drawing circles and scaling with matrix
        glPushMatrix();
        glTranslatef(cx, cy, 0);
        glScalef(1.6f, 0.8f, 1.0f);
        drawFilledCircle(0,0,30);
        glPopMatrix();
    }
}

void drawMushroom(const Mushroom &m) {
    // Stem - rounded rectangle: draw as simple polygon
    setColor(stemColor);
    std::vector<std::pair<int,int>> stem;
    int sx = m.cx - m.stemH/6, ex = m.cx + m.stemH/6;
    stem.push_back({sx, m.cy});
    stem.push_back({ex, m.cy});
    stem.push_back({ex, m.cy + m.stemH});
    stem.push_back({sx, m.cy + m.stemH});
    drawFilledPoly(stem);

    // Cap (ellipse shape). Use scale transforms to make nice cap
    glPushMatrix();
    glTranslatef(m.cx, m.cy + m.stemH, 0);
    glScalef((float)m.capW/100.0f, (float)m.capH/50.0f, 1.0f);
    setColor(mushroomCap);
    drawFilledCircle(0,0,60);
    // cap underside shadow
    setColor({0.28f,0.08f,0.27f});
    drawFilledCircle(0,-18, 40); // note: using overloaded function not present; instead small hack below
    glPopMatrix();

    // cap underside: draw darker arc manually
    setColor({0.28f,0.08f,0.27f});
    int steps = 40;
    glBegin(GL_POLYGON);
    for(int i=0;i<=steps;i++){
        float theta = M_PI * i / steps; // half circle
        float x = (m.capW*0.5f) * cos(theta);
        float y = (m.capH*0.18f) * sin(theta) + m.cy + m.stemH - 15;
        glVertex2f(m.cx + x, y);
    }
    glVertex2f(m.cx - m.capW*0.5f, m.cy + m.stemH - 15);
    glEnd();
}

void drawAstronaut(int baseX, int baseY) {
    // Draw astronaut using polygons and circles; translate model so baseX,baseY form feet baseline center
    int x = baseX, y = baseY;
    // Body rectangle
    setColor(astronautWhite);
    std::vector<std::pair<int,int>> body = {
        {x - AG.bodyW/2, y - AG.bodyH}, {x + AG.bodyW/2, y - AG.bodyH},
        {x + AG.bodyW/2, y - 30}, {x - AG.bodyW/2, y - 30}
    };
    drawFilledPoly(body);

    // Backpack (left of body)
    setColor({0.84f,0.88f,0.92f});
    std::vector<std::pair<int,int>> pack = {
        {x - AG.bodyW/2 - AG.backpackW, y - AG.bodyH + 40},
        {x - AG.bodyW/2 - 10, y - AG.bodyH + 40},
        {x - AG.bodyW/2 - 10, y - 20},
        {x - AG.bodyW/2 - AG.backpackW, y - 20}
    };
    drawFilledPoly(pack);

    // Head (helmet) - circle using midpoint
    int headC_x = x;
    int headC_y = y - AG.bodyH - 20;
    setColor(astronautWhite);
    drawFilledCircle(headC_x, headC_y, AG.headR + 6);

    // Visor (dark) - use filled circle and then overlay to simulate oval
    setColor(visorBlack);
    glPushMatrix();
    glTranslatef(headC_x, headC_y, 0);
    glScalef(1.6f,1.0f,1.0f);
    drawFilledCircle(0, 0, AG.headR - 2);
    glPopMatrix();

    // Left & right arms (simple polygons)
    setColor(astronautWhite);
    // right arm extends to flashlight
    std::vector<std::pair<int,int>> rarm = {
        {x + AG.bodyW/2 - 10, y - AG.bodyH + 60},
        {x + AG.bodyW/2 + 70, y - AG.bodyH + 40},
        {x + AG.bodyW/2 + 60, y - AG.bodyH + 20},
        {x + AG.bodyW/2 - 10, y - AG.bodyH + 40}
    };
    drawFilledPoly(rarm);

    // left arm (tucked)
    std::vector<std::pair<int,int>> larm = {
        {x - AG.bodyW/2 + 10, y - AG.bodyH + 60},
        {x - AG.bodyW/2 - 40, y - AG.bodyH + 50},
        {x - AG.bodyW/2 - 30, y - AG.bodyH + 30},
        {x - AG.bodyW/2 + 10, y - AG.bodyH + 40}
    };
    drawFilledPoly(larm);

    // Legs - animate swing a bit
    int legW = 36;
    int legH = 80;
    int legOffsetX = 36;
    float swing = 0.0f;
    if (walking) swing = 8.0f * sinf(walkFrame * 0.25f);
    // left foot
    glPushMatrix();
    glTranslatef(x - legOffsetX, y - 10, 0);
    glRotatef(-swing,0,0,1);
    setColor(astronautWhite);
    std::vector<std::pair<int,int>> lf = {
        {-legW/2, 0}, {legW/2,0}, {legW/2, -legH}, {-legW/2, -legH}
    };
    drawFilledPoly(lf);
    glPopMatrix();
    // right foot
    glPushMatrix();
    glTranslatef(x + legOffsetX, y - 10, 0);
    glRotatef(swing,0,0,1);
    std::vector<std::pair<int,int>> rf = lf;
    drawFilledPoly(rf);
    glPopMatrix();

    // Flashlight (a small rectangle at the end of right arm) and light cone
    int flx = x + AG.bodyW/2 + 65;
    int fly = y - AG.bodyH + 30;
    setColor({0.8f,0.7f,0.45f});
    std::vector<std::pair<int,int>> fl = {
        {flx-6, fly-6}, {flx+6, fly-6}, {flx+6, fly+6}, {flx-6, fly+6}
    };
    drawFilledPoly(fl);

    // Light cone: semi-transparent yellow polygon
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1.0f,0.92f,0.6f,0.18f);
    glBegin(GL_POLYGON);
    glVertex2i(flx+10, fly+2);
    glVertex2i(flx+240, fly+70);
    glVertex2i(flx+420, fly+120);
    glVertex2i(flx+420, fly+140);
    glVertex2i(flx+240, fly+90);
    glVertex2i(flx+10, fly+22);
    glEnd();
    glDisable(GL_BLEND);

    // Outline components with DDA for crispness (helmet edge & backpack)
    setColor({0.03f,0.03f,0.06f});
    drawLineDDA(headC_x - (AG.headR+6), headC_y, headC_x + (AG.headR+6), headC_y); // simple horizontal
}

// -------------------- OpenGL callbacks --------------------
void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    drawGradientSky();
    drawMoon();
    drawPlanetAndRing(); // ring rotates via ringAngle variable
    drawMountains();
    drawGroundAndRocks();
    // mushrooms
    drawMushroom(mLeft);
    drawMushroom(mRightBig);
    drawMushroom(mRightMed);
    drawMushroom(mRightSmall);
    // astronaut — position controlled by astronautX, astronautY
    drawAstronaut(astronautX, astronautY);
    glutSwapBuffers();
}

void idleFunc() {
    // animate ring
    ringAngle += ringSpeed;
    if (ringAngle > 360.0f) ringAngle -= 360.0f;
    // walking animation frame update
    if (walking) {
        walkFrame++;
        if (walkFrame > 1000000) walkFrame = 0;
    }
    glutPostRedisplay();
}

void reshape(int w,int h) {
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // 1:1 mapping coordinates with origin at top-left -> easier to match pixels
    glOrtho(0, WIN_W, WIN_H, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

// Keyboard controls
void specialKeys(int key, int x, int y) {
    const int step = 12;
    if (key == GLUT_KEY_LEFT) { astronautX -= step; if (astronautX < 80) astronautX = 80; }
    if (key == GLUT_KEY_RIGHT){ astronautX += step; if (astronautX > WIN_W-80) astronautX = WIN_W-80; }
}

void keyboard(unsigned char key, int x, int y) {
    if (key == 27) exit(0); // Esc
    if (key == ' ') walking = !walking;
}

int main(int argc,char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(WIN_W, WIN_H);
    glutCreateWindow("Planet Night Scene - GLUT");

    glClearColor(0.0,0.0,0.0,1.0);
    glPointSize(1.0f);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idleFunc);
    glutSpecialFunc(specialKeys);
    glutKeyboardFunc(keyboard);

    std::cout << "Controls: <- and -> to move astronaut. Space to toggle walking. Esc to quit.\n";

    glutMainLoop();
    return 0;
}
