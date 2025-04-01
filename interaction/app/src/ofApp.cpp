#include "ofApp.h"

void ofApp::setup() {
  ofSetWindowTitle("interaction");
  ofSetFrameRate(30);
  ofSetVerticalSync(true);
  ofBackground(0);
  ofSetWindowShape(800, 600);
  
  // configurar receptor osc
  receiver.setup(OSC_PORT);
}

void ofApp::update() {
  // procesar mensajes osc
  while (receiver.hasWaitingMessages()) {
    ofxOscMessage m;
    receiver.getNextMessage(m);
    
    if (m.getAddress() == "/pose/data") {
      Pose pose;
      pose.id = m.getArgAsInt(0);
      
      // procesar keypoints (17 puntos * 3 valores cada uno)
      for (int i = 1; i < m.getNumArgs(); i += 3) {
        KeyPoint kp;
        kp.x = m.getArgAsFloat(i) * ofGetWidth();
        kp.y = m.getArgAsFloat(i + 1) * ofGetHeight();
        kp.confidence = m.getArgAsFloat(i + 2);
        pose.keypoints.push_back(kp);
      }
      
      // actualizar o añadir la pose
      if (pose.id < poses.size()) {
        poses[pose.id] = pose;
      } else {
        poses.push_back(pose);
      }
    }
  }
}

void ofApp::draw() {
  // dibujar poses
  for (const auto& pose : poses) {
    // dibujar líneas del esqueleto
    ofSetColor(0, 255, 0);  // verde para las líneas
    ofSetLineWidth(2);
    
    for (const auto& [start_idx, end_idx] : SKELETON) {
      if (start_idx >= pose.keypoints.size() || end_idx >= pose.keypoints.size()) {
        continue;
      }
      
      const auto& start = pose.keypoints[start_idx];
      const auto& end = pose.keypoints[end_idx];
      
      if (start.confidence > CONFIDENCE_THRESHOLD && end.confidence > CONFIDENCE_THRESHOLD) {
        ofDrawLine(start.x, start.y, end.x, end.y);
      }
    }
    
    // dibujar puntos
    ofSetColor(255, 0, 0);  // rojo para los puntos
    for (const auto& kp : pose.keypoints) {
      if (kp.confidence > CONFIDENCE_THRESHOLD) {
        ofDrawCircle(kp.x, kp.y, 5);
      }
    }
  }
}