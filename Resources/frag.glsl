#version 450 core

layout (location = 0) in vec3 fragPos;

layout (location = 1) in vec2 fragUV;

layout (location = 2) in vec3 fragNor;

layout (location = 3) in vec3 fragTan;

layout (location = 4) in vec3 fragBitan;

layout (location = 0) out vec4 outFragColor;

layout (set = 0, binding = 0) uniform MVP {
    mat4 model;
} mvp;

layout (set = 0, binding = 1) uniform sampler2D textureSampler;

layout (set = 0, binding = 2) uniform sampler2D normalSampler;

layout (set = 0, binding = 3) uniform sampler2D specSampler;

layout (push_constant) uniform VP {
    mat4 view;
    mat4 projection;
} vp;

const vec3 lightPos = vec3(0.0, 0.0, 300.0);

const float lightIntensity = 1.0f;

const float shininess = 50.0;

void main()
{
    mat3 TBN = transpose(mat3(
        fragTan,
        fragBitan,
        fragNor
    ));

    vec3 normapFragNor = normalize(texture(normalSampler, fragUV).rgb * 2.0 - 1.0);

    vec3 lightDirectionTangSpace = TBN * (lightPos - fragPos);

    float dotProduct = dot(normalize(lightDirectionTangSpace), normalize(normapFragNor));

    float meshNormalDotProduct = dot(normalize(lightDirectionTangSpace), normalize(fragNor));

    float diffuse = min(max(dotProduct, 0.0), 1.0);


    float specular = pow(diffuse, shininess);

    vec4 texelColor = texture(textureSampler, fragUV);

    vec3 specularColor = vec3(texture(specSampler, fragUV));


    float specResultColorComponent = min(1.0, diffuse);

    outFragColor = vec4(diffuse * vec3(texelColor)  , texelColor.a); // For Spec Map: diffuse * vec3(texelColor) + (specular * specularColor)
}