#pragma once

#include "../drawable.h"
#include "../../util/rate.h"
#include "../../util/hash.h"

#if __has_include(<TFT_eSPI.h>)

class Triangles
{
    public:
        Dimension2D<unsigned> dimensions;
        Point2D<int> point;
        Color darkColor = DARK_GRAY;
        Color lightColor = TURQUOISE;
        Rate rate = Rate(0.3f);


        Triangles(unsigned numAgents=0);

        void draw(TFT_eSprite& sprite);
    
    private:
        struct Agent
        {
            Point2D<float> position;
            Point2D<float> velocity;

            Agent(float px, float py, float vx, float vy);
        };

        struct Connection
        {
            unsigned agent1;
            unsigned agent2;

            Connection(unsigned agent1, unsigned agent2);

            bool operator==(const Connection& other) const;
        };

        uint64_t lastTime;
        std::vector<Agent> agents;

        void zone(Agent& agent);
        int closest(Agent& agent, std::vector<bool>& ignore);
        bool connected(Connection& connection, std::vector<Connection>& connections);
};

#endif

