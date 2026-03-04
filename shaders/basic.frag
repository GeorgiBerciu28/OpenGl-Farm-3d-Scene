#version 410 core

in vec3 fNormal;
in vec4 fPosEye;
in vec2 fTexCoords;


in vec4 fragPosLightSpace;

out vec4 fColor;


uniform vec3 lightDir;
uniform vec3 lightColor;


uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;

uniform bool enableFog;

uniform bool nightMode;


uniform sampler2D shadowMap;

struct PointLight {
    vec3 position;      
    vec3 color;
    float constant;
    float linear;
    float quadratic;
};
#define MAX_POINT_LIGHTS 8
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform int numPointLights;


vec3 ambient;
float ambientStrength;
vec3 diffuse;
vec3 specular;
float specularStrength = 0.5f;
float shininess = 32.0f;
uniform bool isSky;
void computeLightComponents()
{    
    ambientStrength = nightMode ? 0.08 : 0.2; 
    vec3 cameraPosEye = vec3(0.0f);

    vec3 normalEye = normalize(fNormal);  
    vec3 lightDirN = normalize(lightDir);
    vec3 viewDirN = normalize(cameraPosEye - fPosEye.xyz);
    vec3 halfVector = normalize(lightDirN + viewDirN);

    ambient = ambientStrength * lightColor;
    diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor;

    float specCoeff = pow(max(dot(normalEye, halfVector), 0.0f), shininess);
    specular = specularStrength * specCoeff * lightColor;

   
}

vec3 computePointLight(PointLight light, vec3 normalEye, vec3 viewDirN)
{
    
    vec3 lightDirN = normalize(light.position - fPosEye.xyz);

    
    float dist = length(light.position - fPosEye.xyz);

   
    float att = 1.0 / (light.constant +
                       light.linear * dist +
                       light.quadratic * dist * dist);

    
    vec3 ambientP = att * ambientStrength * light.color;

    float diff = max(dot(normalEye, lightDirN), 0.0);
    vec3 diffuseP = att * diff * light.color;

    
    vec3 halfVector = normalize(lightDirN + viewDirN);
    float spec = pow(max(dot(normalEye, halfVector), 0.0), shininess);
    vec3 specularP = att * specularStrength * spec * light.color;

    return ambientP + diffuseP + specularP;
}

float computeShadow()
{
    
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    
    projCoords = projCoords * 0.5 + 0.5;

    
   if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0 ||
        projCoords.z > 1.0)
        return 0.0;

    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;

    float bias = max(0.01 * (1.0 - dot(normalize(fNormal), normalize(lightDir))), 0.002);

    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;

    return shadow;
}
float computeFog()
{
 float fogDensity = 0.05f;
 float fragmentDistance = length(fPosEye.xyz);

 float fogFactor = exp(-pow(fragmentDistance * fogDensity, 2));

 return clamp(fogFactor, 0.0f, 1.0f);
}

void main()
{
    if (isSky) {
 	vec3 skyColor = texture(diffuseTexture, fTexCoords).rgb; 
    if (!enableFog) { 
       fColor = vec4(skyColor, 1.0); 
       return; 
    } 
    float fogFactor = computeFog(); 
    fogFactor = clamp(fogFactor, 0.2, 1.0); 
    vec3 fogColor; 
    if (nightMode) 
     fogColor = vec3(0.05, 0.07, 0.1); 
      else 
    fogColor = vec3(0.5, 0.5, 0.5); 
    vec3 finalColor = mix(fogColor, skyColor, fogFactor); 
    fColor = vec4(finalColor, 1.0); 
    return; 
 }
    computeLightComponents();

    ambient *= texture(diffuseTexture, fTexCoords).rgb;
    diffuse *= texture(diffuseTexture, fTexCoords).rgb;
    specular *= texture(specularTexture, fTexCoords).rgb;
    vec3 texDiffuse = texture(diffuseTexture, fTexCoords).rgb;

    
    float shadow = computeShadow();
    vec3 color;
   if (!nightMode)
    {
        color = ambient +
                (1.0 - shadow) * diffuse +
                (1.0 - shadow) * specular;
    }
    else
    {
                color = ambient +
                0.3 * (1.0 - shadow) * diffuse +
                0.3 * (1.0 - shadow) * specular;

        vec3 normalEye = normalize(fNormal);
        vec3 viewDirN = normalize(-fPosEye.xyz);

        for (int i = 0; i < numPointLights; i++)
        {
            vec3 pLight = computePointLight(pointLights[i], normalEye, viewDirN);
            pLight *= texDiffuse;
            color += pLight;
        }
    }
    color = min(color, vec3(1.0));
    fColor = vec4(color, 1.0f);
    if (enableFog)
    {
        float fogFactor = computeFog();

        vec4 fogColor;
        if (nightMode)
            fogColor = vec4(0.05, 0.07, 0.1, 1.0);  
        else
            fogColor = vec4(0.5, 0.5, 0.5, 1.0);    

        fColor = fogColor * (1.0 - fogFactor) + fColor * fogFactor;
    }
}

