#version 330
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform vec3 viewPos;


struct Light {
    vec3 Position;
    vec3 Color;
    
    float Linear;
    float Quadratic;
};
const int NR_LIGHTS = 1;
uniform Light lights[NR_LIGHTS];


void main()
{             
    vec3 FragPos = texture(gPosition,   TexCoords).rgb;
    vec3 Normal  = texture(gNormal,     TexCoords).rgb; // TODO: optimize decode
    vec4 DiSpec  = texture(gAlbedoSpec, TexCoords);
    vec3 Diffuse = DiSpec.rgb;
    float Specular = 0.3;//DiSpec.a; // hard-coded specular material
    
    vec3 lighting  = Diffuse * 0.8; // hard-coded ambient component
    vec3 viewDir  = normalize(viewPos - FragPos);

    // Then calculate lighting as usual

    // Diffuse
    vec3 lightDir = -normalize(lights[0].Position - FragPos);
    vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse; //lights[0].Color;
    // Specular
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(Normal, halfwayDir), 0.0), 16.0); // hard coded specular power
    vec3 specular = lights[0].Color * spec * Specular;
    // Attenuation
    float distance = length(lights[0].Position - FragPos);
    float attenuation = 1.0 / (1.0 + lights[0].Linear * distance + lights[0].Quadratic * distance * distance);
    lighting += (diffuse + specular)*attenuation;
    FragColor = vec4( max(dot(Normal, lightDir), 0.0) ,0,0,1);
    FragColor = vec4(lighting,1);
//    FragColor = vec4(Normal,1);
    
}