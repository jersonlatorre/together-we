#pragma once

#include "ofMain.h"
#include "ofxOsc.h"
#include <vector>

struct KeyPoint {
    float x;
    float y;
    float confidence;
};

struct Pose {
    int id;
    std::vector<KeyPoint> keypoints;
};

class ofApp : public ofBaseApp {
public:
    void setup();
    void update();
    void draw();

private:
    ofxOscReceiver receiver;
    std::vector<Pose> poses;
    static constexpr int OSC_PORT = 12345;
    static constexpr float CONFIDENCE_THRESHOLD = 0.5;
    
    // definición del esqueleto (pares de índices)
    const std::vector<std::pair<int, int>> SKELETON = {
        {0, 1},   // nariz -> ojo izquierdo
        {0, 2},   // nariz -> ojo derecho
        {1, 3},   // ojo izquierdo -> oreja izquierda
        {2, 4},   // ojo derecho -> oreja derecha
        {5, 7},   // hombro izquierdo -> codo izquierdo
        {7, 9},   // codo izquierdo -> muñeca izquierda
        {6, 8},   // hombro derecho -> codo derecho
        {8, 10},  // codo derecho -> muñeca derecha
        {5, 6},   // hombro izquierdo -> hombro derecho
        {5, 11},  // hombro izquierdo -> cadera izquierda
        {6, 12},  // hombro derecho -> cadera derecha
        {11, 12}, // cadera izquierda -> cadera derecha
        {11, 13}, // cadera izquierda -> rodilla izquierda
        {13, 15}, // rodilla izquierda -> tobillo izquierdo
        {12, 14}, // cadera derecha -> rodilla derecha
        {14, 16}  // rodilla derecha -> tobillo derecho
    };
};
