#include "triangles.h"
#include "../../hal/hal.h"
#include <limits>

#if __has_include(<TFT_eSPI.h>)

Triangles::Triangles(unsigned numAgents)
{
    agents.reserve(numAgents);
    for (unsigned i = 0; i < numAgents; i++)
    {
        agents.emplace_back(
            random(0, 1001) / 1000.0f,
            random(0, 1001) / 1000.0f,
            random(10, 101) / 1000.0f,
            random(10, 101) / 1000.0f
        );

        if (random(0, 2))
            agents.back().velocity.x *= -1;
        if (random(0, 2))
            agents.back().velocity.y *= -1;
    }

    lastTime = sysTime();
}

void Triangles::draw(TFT_eSprite& sprite)
{
    uint64_t time = sysTime();
    float step = (time - lastTime) / (float) 1e6;
    if (step >= 1)
        step = 0;
    
    lastTime = time;

    for (Agent& agent : agents)
    {
        agent.position.x += agent.velocity.x * step;
        agent.position.y += agent.velocity.y * step;
        zone(agent);
    }

    std::vector<Connection> connections;
    std::vector<bool> ignore(agents.size(), false);
    for (unsigned i = 0; i < agents.size(); i++)
    {
        ignore[i] = true;
        for (unsigned j = 0; j < 3; j++)
        {
            int agent2 = closest(agents[i], ignore);
            if (agent2 != -1)
            {
                connections.emplace_back(i, agent2);
                ignore[agent2] = true;
            }
        }

        for (unsigned j = 0; j < ignore.size(); j++)
            ignore[j] = false;
    }

    float stage = rate.getStageCos();
    for (Connection& connection : connections)
    {
        Agent& agent1 = agents[connection.agent1];
        Agent& agent2 = agents[connection.agent2];

        float d1 = 1 - fabs(stage - agent1.position.y);
        float d2 = 1 - fabs(stage - agent2.position.y);
        float bias = powf((d1 + d2) / 2.0f, 5);

        sprite.drawLine(
            (int) (agent1.position.x * dimensions.width) + point.x,
            (int) (agent1.position.y * dimensions.height) + point.y,
            (int) (agent2.position.x * dimensions.width) + point.x,
            (int) (agent2.position.y * dimensions.height) + point.y,
            darkColor.blend(lightColor, bias).as16Bit()
        );
    }
}

Triangles::Agent::Agent(float px, float py, float vx, float vy)
{
    position.set(px, py);
    velocity.set(vx, vy);
}

Triangles::Connection::Connection(unsigned agent1, unsigned agent2) : agent1(agent1), agent2(agent2)
{

}

bool Triangles::Connection::operator==(const Triangles::Connection& other) const
{
    return (agent1 == other.agent1 && agent2 == other.agent2) || (agent1 == other.agent2 && agent2 == other.agent1);
}

void Triangles::zone(Triangles::Agent& agent)
{
    if (agent.position.x < 0)
    {
        agent.position.x = -agent.position.x;
        agent.velocity.x = -agent.velocity.x;
    }
    else if (agent.position.x > 1)
    {
        agent.position.x = 2 - agent.position.x;
        agent.velocity.x = -agent.velocity.x;
    }

    if (agent.position.y < 0)
    {
        agent.position.y = -agent.position.y;
        agent.velocity.y = -agent.velocity.y;
    }
    else if (agent.position.y > 1)
    {
        agent.position.y = 2 - agent.position.y;
        agent.velocity.y = -agent.velocity.y;
    }
}

int Triangles::closest(Triangles::Agent& agent, std::vector<bool>& ignore)
{
    int idx = -1;
    float min = std::numeric_limits<float>::max();
    for (unsigned i = 0; i < agents.size(); i++)
    {
        float dist = agent.position.distance(agents[i].position);
        if (dist < min && !ignore[i])
        {
            idx = i;
            min = dist;
        }
    }
    
    return idx;
}

bool Triangles::connected(Triangles::Connection& connection, std::vector<Triangles::Connection>& connections)
{
    for (Connection& conn : connections)
    {
        if (connection == conn)
            return true;
    }

    return false;
}

#endif
