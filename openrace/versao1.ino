#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>

// WiFi and MQTT settings
const char* ssid = "moto g(7) 4767";
const char* password = "arduino351";
const char* mqtt_server = "broker.mqtt-dashboard.com";
#define MQTTPORT 1883

// LED strip settings
#define PIN_LED        D3
#define MAXLED         300
#define NPIXELS        MAXLED

// Race settings
#define LOOP_MAX       5
#define MAX_CARS       4

// MQTT topics
#define TOPIC_PREFIX   "trackXX/"

// Car struct to hold car-specific data
struct Car {
    float speed;
    float dist;
    byte loop;
    uint32_t color;
    char topic[20];
};

// Array of cars
Car cars[MAX_CARS];

// Other variables
Adafruit_NeoPixel track = Adafruit_NeoPixel(NPIXELS, PIN_LED, NEO_GRB + NEO_KHZ800);
WiFiClient espClient;
PubSubClient client(espClient);

float ACEL = 0.2;
float kf = 0.015;
byte leader = 0;
byte draworder = 0;
int tdelay = 5;

void setup() {
    Serial.begin(115200);
    track.begin();
    setup_wifi();
    client.setServer(mqtt_server, MQTTPORT);
    client.setCallback(callback);

    // Initialize cars
    initializeCars();

    start_race();
}

void initializeCars() {
    const char* carNames[] = {"red", "green", "blue", "yellow"};
    uint32_t carColors[] = {track.Color(255,0,0), track.Color(0,255,0), track.Color(0,0,255), track.Color(255,255,0)};

    for (int i = 0; i < MAX_CARS; i++) {
        cars[i].speed = 0;
        cars[i].dist = 0;
        cars[i].loop = 0;
        cars[i].color = carColors[i];
        sprintf(cars[i].topic, "%s%scar", TOPIC_PREFIX, carNames[i]);
        client.subscribe(cars[i].topic);
    }
}

void callback(char* topic, byte* payload, unsigned int length) {
    for (int i = 0; i < MAX_CARS; i++) {
        if (strcmp(topic, cars[i].topic) == 0) {
            cars[i].speed += ACEL;
            break;
        }
    }
}

void setup_wifi() {
    // WiFi setup code (unchanged)
}

void reconnect() {
    // MQTT reconnection code (unchanged)
}

void start_race() {
    // Race start animation code (unchanged)
}

void draw_car(int carIndex) {
    for (int i = 0; i <= cars[carIndex].loop; i++) {
        uint32_t color = track.Color(
            ((cars[carIndex].color >> 16) & 0xFF) - i * 20,
            ((cars[carIndex].color >> 8) & 0xFF) - i * 20,
            (cars[carIndex].color & 0xFF) - i * 20
        );
        track.setPixelColor(((word)cars[carIndex].dist % NPIXELS) + i, color);
    }
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    for (int i = 0; i < NPIXELS; i++) {
        track.setPixelColor(i, track.Color(0,0,0));
    }

    for (int i = 0; i < MAX_CARS; i++) {
        cars[i].speed -= cars[i].speed * kf;
        cars[i].dist += cars[i].speed;

        if (cars[i].dist > NPIXELS * cars[i].loop) {
            cars[i].loop++;
        }

        if (cars[i].loop > LOOP_MAX) {
            // End race and reset
            for (int j = 0; j < NPIXELS; j++) {
                track.setPixelColor(j, cars[i].color);
            }
            track.show();
            delay(2000);
            resetRace();
            start_race();
            return;
        }
    }

    // Determine leader
    leader = 0;
    for (int i = 1; i < MAX_CARS; i++) {
        if (cars[i].dist > cars[leader].dist) {
            leader = i;
        }
    }

    // Draw cars
    for (int i = 0; i < MAX_CARS; i++) {
        draw_car(i);
    }

    track.show();
    delay(tdelay);
}

void resetRace() {
    for (int i = 0; i < MAX_CARS; i++) {
        cars[i].speed = 0;
        cars[i].dist = 0;
        cars[i].loop = 0;
    }
}
