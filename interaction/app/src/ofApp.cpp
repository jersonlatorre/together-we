#include "ofApp.h"

void ofApp::setup() {
  ofSetWindowTitle("interaction");
  ofSetFrameRate(0); // detener el bucle automático
  ofSetVerticalSync(true);
  ofSetWindowShape(800, 600);

  // configurar receptor osc
  receiver.setup(OSC_PORT);

  // configurar FBO
  ofFboSettings settings;
  settings.width = ofGetWidth();
  settings.height = ofGetHeight();
  settings.numSamples = 4; // activar antialiasing con 4 muestras
  fbo.allocate(settings);
}

void ofApp::update() {
  // limpiar poses del frame anterior
  poses.clear();
  has_new_data = false;

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

      // añadir la pose si no excedemos el máximo
      if (poses.size() < MAX_POSES) {
        poses.push_back(pose);
        has_new_data = true;
      }
    }
  }

  // si recibimos nuevos datos, actualizar el FBO
  if (has_new_data) {
    updateFbo();
  }
}

void ofApp::updateFbo() {
  // dibujar en el FBO
  fbo.begin();
  ofBackground(0);

  // dibujar poses
  for (const auto &pose : poses) {
    // dibujar líneas del esqueleto
    ofSetColor(0, 255, 0); // verde para las líneas
    ofSetLineWidth(2);

    // dibujar solo la línea que conecta las manos a través de los brazos y
    // hombros
    const int left_hand = 9;      // índice de la mano izquierda
    const int left_elbow = 7;     // índice del codo izquierdo
    const int left_shoulder = 5;  // índice del hombro izquierdo
    const int right_shoulder = 6; // índice del hombro derecho
    const int right_elbow = 8;    // índice del codo derecho
    const int right_hand = 10;    // índice de la mano derecha

    if (pose.keypoints.size() > right_hand) {
      const auto &left_hand_kp = pose.keypoints[left_hand];
      const auto &left_elbow_kp = pose.keypoints[left_elbow];
      const auto &left_shoulder_kp = pose.keypoints[left_shoulder];
      const auto &right_shoulder_kp = pose.keypoints[right_shoulder];
      const auto &right_elbow_kp = pose.keypoints[right_elbow];
      const auto &right_hand_kp = pose.keypoints[right_hand];
      const auto &left_eye_kp = pose.keypoints[1];
      const auto &right_eye_kp = pose.keypoints[2];

      if (left_hand_kp.confidence > CONFIDENCE_THRESHOLD &&
          left_elbow_kp.confidence > CONFIDENCE_THRESHOLD &&
          left_shoulder_kp.confidence > CONFIDENCE_THRESHOLD &&
          right_shoulder_kp.confidence > CONFIDENCE_THRESHOLD &&
          right_elbow_kp.confidence > CONFIDENCE_THRESHOLD &&
          right_hand_kp.confidence > CONFIDENCE_THRESHOLD) {

        ofDrawLine(left_hand_kp.x, left_hand_kp.y, left_elbow_kp.x,
                   left_elbow_kp.y);
        ofDrawLine(left_elbow_kp.x, left_elbow_kp.y, left_shoulder_kp.x,
                   left_shoulder_kp.y);
        ofDrawLine(left_shoulder_kp.x, left_shoulder_kp.y, right_shoulder_kp.x,
                   right_shoulder_kp.y);
        ofDrawLine(right_shoulder_kp.x, right_shoulder_kp.y, right_elbow_kp.x,
                   right_elbow_kp.y);
        ofDrawLine(right_elbow_kp.x, right_elbow_kp.y, right_hand_kp.x,
                   right_hand_kp.y);
      }

      // dibujar línea entre ojos si tienen suficiente confianza
      if (left_eye_kp.confidence > CONFIDENCE_THRESHOLD ||
          right_eye_kp.confidence > CONFIDENCE_THRESHOLD) {
        ofDrawLine(left_eye_kp.x, left_eye_kp.y, right_eye_kp.x, right_eye_kp.y);
      }
    }
  }
  fbo.end();
}

void ofApp::draw() {
  // solo dibujar el FBO en la pantalla
  fbo.draw(0, 0);
}