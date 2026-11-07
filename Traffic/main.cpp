#include <GL/glut.h>
#include <math.h>

void display() {
    glClearColor(0.9f, 0.95f, 1.0f, 1.0f); // Light Sky Background
    glClear(GL_COLOR_BUFFER_BIT);

    // 1. The Pole (Using LINES)
    glColor3f(0.2f, 0.2f, 0.2f); // Dark Grey
    glLineWidth(10);             // Thick pole
    glBegin(GL_LINES);
        glVertex2f(0.0f, -0.6f); // Top of pole (connected to box)
        glVertex2f(0.0f, -1.0f); // Bottom of pole
    glEnd();

    // 2. Main Casing Box (Using QUADS)
    glColor3f(0.1f, 0.1f, 0.1f); // Almost Black
    glBegin(GL_QUADS);
        glVertex2f(-0.25f,  0.6f); // Top Left
        glVertex2f( 0.25f,  0.6f); // Top Right
        glVertex2f( 0.25f, -0.6f); // Bottom Right
        glVertex2f(-0.25f, -0.6f); // Bottom Left
    glEnd();

    // 3. Red Light (Top) - Using POLYGON Loop
    glColor3f(1.0f, 0.0f, 0.0f); // Red Color
    glBegin(GL_POLYGON);
    for(int i=0; i<360; i++) {
        float theta = i * 3.1416 / 180;
        // Center at (0.0, 0.4), Radius 0.12
        glVertex2f(0.0f + 0.12f * cos(theta), 0.4f + 0.12f * sin(theta));
    }
    glEnd();

    // 4. Yellow Light (Middle) - Using POLYGON Loop
    glColor3f(1.0f, 0.8f, 0.0f); // Yellow Color
    glBegin(GL_POLYGON);
    for(int i=0; i<360; i++) {
        float theta = i * 3.1416 / 180;
        // Center at (0.0, 0.0), Radius 0.12
        glVertex2f(0.0f + 0.12f * cos(theta), 0.0f + 0.12f * sin(theta));
    }
    glEnd();

    // 5. Green Light (Bottom) - Using POLYGON Loop
    glColor3f(0.0f, 1.0f, 0.0f); // Green Color
    glBegin(GL_POLYGON);
    for(int i=0; i<360; i++) {
        float theta = i * 3.1416 / 180;
        // Center at (0.0, -0.4), Radius 0.12
        glVertex2f(0.0f + 0.12f * cos(theta), -0.4f + 0.12f * sin(theta));
    }
    glEnd();

    // 6. Visors/Caps above lights (Using TRIANGLES)
    glColor3f(0.15f, 0.15f, 0.15f); // Darker Grey for visors

    // Top Visor
    glBegin(GL_TRIANGLES);
        glVertex2f(-0.15f, 0.55f);
        glVertex2f( 0.15f, 0.55f);
        glVertex2f( 0.0f,  0.65f); // Pointing up
    glEnd();

    // Middle Visor
    glBegin(GL_TRIANGLES);
        glVertex2f(-0.15f, 0.15f);
        glVertex2f( 0.15f, 0.15f);
        glVertex2f( 0.0f,  0.25f);
    glEnd();

    // Bottom Visor
    glBegin(GL_TRIANGLES);
        glVertex2f(-0.15f, -0.25f);
        glVertex2f( 0.15f, -0.25f);
        glVertex2f( 0.0f,  -0.15f);
    glEnd();

    // 7. Corner Bolts (Using POINTS)
    glColor3f(1.0f, 1.0f, 1.0f); // White/Silver bolts
    glPointSize(6);              // Large dots
    glBegin(GL_POINTS);
        glVertex2f(-0.20f, 0.55f);  // Top Left
        glVertex2f( 0.20f, 0.55f);  // Top Right
        glVertex2f(-0.20f, -0.55f); // Bottom Left
        glVertex2f( 0.20f, -0.55f); // Bottom Right
    glEnd();

    glFlush();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutCreateWindow("Traffic Signal Design");
    glutInitWindowSize(320, 320);
    glutDisplayFunc(display);
    glutMainLoop();
    return 0;
}
