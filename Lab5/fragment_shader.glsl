#version 460 core
in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

uniform vec3 modelColor;
uniform vec3 viewPos;

void main() {
    // Серый цвет модели
    vec3 objectColor = modelColor;
    
    // Свет от камеры
    vec3 lightPos = viewPos;
    vec3 lightColor = vec3(1.0, 1.0, 1.0);
    
    // Ambient (фоновое освещение)
    vec3 ambient = 0.25 * lightColor;
    
    // Diffuse (рассеянное)
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular (блики)
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 8.0);
    vec3 specular = 0.1 * spec * lightColor;
    
    // Итоговый цвет
    vec3 result = (ambient + diffuse + specular) * objectColor;
    
    // Ограничение максимальной яркости
    result = min(result, vec3(1.0));
    
    FragColor = vec4(result, 1.0);
}