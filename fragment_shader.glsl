#version 330 core

out vec4 FragColor;

in vec3 fragNormal;
in vec3 fragPos;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform vec3 frontColor;
uniform vec3 backColor;
uniform bool isBackFace;

void main()
{
    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(lightPos - fragPos);
    
    // Определяем, является ли это задней стороной
    vec3 viewDir = normalize(viewPos - fragPos);
    float dotProduct = dot(norm, viewDir);
    
    // Выбираем цвет в зависимости от ориентации грани
    vec3 baseColor;
    if (dotProduct < 0.0) {
        // Задняя сторона - используем синий цвет
        baseColor = backColor;
    } else {
        // Передняя сторона - используем оранжевый цвет
        baseColor = frontColor;
    }
    
    // Расчет освещения
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 result = diff * lightColor * baseColor;
    
    // Добавляем небольшое окружающее освещение
    vec3 ambient = 0.1 * baseColor;
    result += ambient;

    FragColor = vec4(result, 1.0f);
}