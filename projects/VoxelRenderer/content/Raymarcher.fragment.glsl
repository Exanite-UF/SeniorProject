#version 460 core

in vec2 uv;

out vec4 out_color;

const float positiveInfinity = 1.0 / 0.0;
const float negativeInfinity = -1.0 / 0.0;

float sdfBox(vec3 position, vec3 extents)
{
    vec3 q = abs(position) - extents;
    return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
}

float sdfSphere(vec3 position, float radius)
{
    return length(position) - radius;
}

float sdfScene(vec3 rayPosition)
{
    float distance = positiveInfinity;

    distance = min(sdfBox(rayPosition - vec3(-1, -1, 0), vec3(1, 1, 1)), distance);
    distance = min(sdfBox(rayPosition - vec3(1, 1, 0), vec3(1, 1, 1)), distance);
    distance = min(sdfSphere(rayPosition - vec3(-1, 1, 5), 1), distance);
    distance = min(sdfSphere(rayPosition - vec3(20, -20, 50), 50), distance);

    return distance;
}

void main()
{
    const float minTravelDistance = 0.01;
    const float maxTravelDistance = 50;
    const int maxIterations = 100;

    const vec3 skyColor = vec3(0, 0, 1);
    const vec3 objectColor = vec3(1, 0, 0);

    vec2 uv = (gl_FragCoord.xy * 2 - vec2(800, 600)) / 600;

    vec3 rayOrigin = vec3(0, 0, -10);

    vec3 rayDirection = normalize(vec3(uv, 1));
    vec3 color = skyColor;

    float distanceTraveled = 0;
    for (int i = 0; i < maxIterations; i++)
    {
        vec3 position = rayOrigin + rayDirection * distanceTraveled;
        float minDistance = sdfScene(position);

        distanceTraveled += minDistance;

        if (minDistance < minTravelDistance)
        {
            color = objectColor;
            break;
        }

        if (distanceTraveled > maxTravelDistance)
        {
            distanceTraveled = maxTravelDistance;

            break;
        }
    }

    float p = max(min(distanceTraveled / maxTravelDistance, 1), 0);
    p = pow(p, 0.9);

    color = p * skyColor + (1 - p) * objectColor;
    out_color = vec4(color, 1);
}
