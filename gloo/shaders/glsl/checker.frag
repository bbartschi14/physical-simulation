#version 330 core

out vec4 frag_color;

struct AmbientLight {
    bool enabled;
    vec3 ambient;
};

struct PointLight {
    bool enabled;
    vec3 position;
    vec3 diffuse;
    vec3 specular;
    vec3 attenuation;
};

struct DirectionalLight {
    bool enabled;
    vec3 direction;
    vec3 diffuse;
    vec3 specular;
};
struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

in vec3 world_position;
in vec3 world_normal;
in vec2 tex_coord;

uniform vec3 camera_position;

uniform Material material1; // material properties of the object
uniform Material material2; // material properties of the object

uniform AmbientLight ambient_light;
uniform PointLight point_light; 
uniform DirectionalLight directional_light;
vec3 CalcAmbientLight(Material material);
vec3 CalcPointLight(vec3 normal, vec3 view_dir, Material material);
vec3 CalcDirectionalLight(vec3 normal, vec3 view_dir, Material material);

void main() {
    vec3 normal = normalize(world_normal);
    vec3 view_dir = normalize(camera_position - world_position);

    float width = 40.0;
    Material material;
    if ( mod( floor(tex_coord.x * width) + floor(tex_coord.y * width) , 2 ) == 0) {
        material = material1;
    } else {
        material = material2;
    }

    frag_color = vec4(0.0);

    if (ambient_light.enabled) {
        frag_color += vec4(CalcAmbientLight(material), 1.0) * 2.0;
    }
    
    if (point_light.enabled) {
        frag_color += vec4(CalcPointLight(normal, view_dir, material), 1.0);
    }

    if (directional_light.enabled) {
        frag_color += vec4(CalcDirectionalLight(normal, view_dir, material), 1.0);
    }

}

vec3 GetAmbientColor(Material material) {
    return material.ambient;
}

vec3 GetDiffuseColor(Material material) {
    return material.diffuse;
}

vec3 GetSpecularColor(Material material) {
    return material.specular;
}

vec3 CalcAmbientLight(Material material) {
    return ambient_light.ambient * GetAmbientColor(material);
}

vec3 CalcPointLight(vec3 normal, vec3 view_dir, Material material) {
    PointLight light = point_light;
    vec3 light_dir = normalize(light.position - world_position);

    float diffuse_intensity = max(dot(normal, light_dir), 0.0);
    vec3 diffuse_color = diffuse_intensity * light.diffuse * GetDiffuseColor(material);

    vec3 reflect_dir = reflect(-light_dir, normal);
    float specular_intensity = pow(
        max(dot(view_dir, reflect_dir), 0.0), material.shininess);
    vec3 specular_color = specular_intensity * 
        light.specular * GetSpecularColor(material);

    float distance = length(light.position - world_position);
    float attenuation = 1.0 / (light.attenuation.x + 
        light.attenuation.y * distance + 
        light.attenuation.z * (distance * distance));

    return attenuation * (diffuse_color + specular_color);
}

vec3 CalcDirectionalLight(vec3 normal, vec3 view_dir, Material material) {
    DirectionalLight light = directional_light;
    vec3 light_dir = normalize(-light.direction);
    float diffuse_intensity = max(dot(normal, light_dir), 0.0);
    vec3 diffuse_color = diffuse_intensity * light.diffuse * GetDiffuseColor(material);

    vec3 reflect_dir = reflect(-light_dir, normal);
    float specular_intensity = pow(
        max(dot(view_dir, reflect_dir), 0.0), material.shininess);
    vec3 specular_color = specular_intensity * 
        light.specular * GetSpecularColor(material);

    vec3 final_color = diffuse_color + specular_color;
    return final_color;
}

