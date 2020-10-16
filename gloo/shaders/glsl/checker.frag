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
struct Texture {
    
    bool texture_on;
    float tile_size;
    sampler2D map;
    bool normal_on;
    sampler2D normal_map;
    bool visualize_normals;
};

in vec3 world_position;
in vec3 world_normal;
in vec2 tex_coord;
in vec3 world_tangent;

uniform vec3 camera_position;

uniform Texture tex;

uniform Material material1; // material properties of the object
uniform Material material2; // material properties of the object


uniform AmbientLight ambient_light;
uniform PointLight point_light; 
uniform DirectionalLight directional_light;
vec3 CalcAmbientLight(Material material, Texture tex);
vec3 CalcPointLight(vec3 normal, vec3 view_dir, Material material, Texture tex);
vec3 CalcDirectionalLight(vec3 normal, vec3 view_dir, Material material, Texture tex);

void main() {
    frag_color = vec4(0.0);

    vec3 normal = normalize(world_normal);
    if (tex.normal_on) {
        vec3 tangent = normalize(world_tangent);
        tangent = normalize(tangent - dot(tangent, normal) * normal);
        vec3 bitangent = cross(normal, tangent);
        vec3 bump_normal = (texture(tex.normal_map, tex_coord*tex.tile_size).rgb * 2.0 - vec3(1.0,1.0,1.0));
        mat3 TBN = mat3(tangent, bitangent, normal);
        normal = normalize(TBN * bump_normal);

    }

    vec3 view_dir = normalize(camera_position - world_position);

    float width = 40.0;
    Material material;
    if (tex.texture_on) {
        material = material1;
    } else {
        if ( mod( floor(tex_coord.x * width) + floor(tex_coord.y * width) , 2 ) == 0) {
            material = material1;
        } else {
            material = material2;
        }
    }
    
    
    
    if (ambient_light.enabled) {
        frag_color += vec4(CalcAmbientLight(material, tex), 1.0) * 2.0;
    }
    
    if (point_light.enabled) {
       frag_color += vec4(CalcPointLight(normal, view_dir, material, tex), 1.0);
    }
    
    if (directional_light.enabled) {
        frag_color += vec4(CalcDirectionalLight(normal, view_dir, material, tex), 1.0);
    }
    
    if (tex.visualize_normals) {
        frag_color = vec4((normal)/2.0 + vec3(.5,.5,.5),1.0)/2.0;
    }
        
}

vec3 CalcAmbientLight(Material material, Texture tex) {
    vec3 ambient;

    if (tex.texture_on) {
        vec4 color = texture(tex.map, tex_coord*tex.tile_size);
        ambient = color.rgb;
    } else {
        ambient = material.ambient;
        
    }
    return ambient_light.ambient * ambient;
}

vec3 CalcPointLight(vec3 normal, vec3 view_dir, Material material, Texture tex) {
    vec3 diffuse;
    vec3 specular;
    if (tex.texture_on) {
        vec4 color = texture(tex.map, tex_coord*tex.tile_size);
        diffuse = color.rgb;
        specular = color.rgb;
    } else {
        diffuse = material.diffuse;
        specular = material.specular;
    }

    PointLight light = point_light;
    vec3 light_dir = normalize(light.position - world_position);

    float diffuse_intensity = max(dot(normal, light_dir), 0.0);
    vec3 diffuse_color = diffuse_intensity * light.diffuse * diffuse;

    vec3 reflect_dir = reflect(-light_dir, normal);
    float specular_intensity = pow(
        max(dot(view_dir, reflect_dir), 0.0), material.shininess);
    vec3 specular_color = specular_intensity * 
        light.specular * specular;

    float distance = length(light.position - world_position);
    float attenuation = 1.0 / (light.attenuation.x + 
        light.attenuation.y * distance + 
        light.attenuation.z * (distance * distance));

    return attenuation * (diffuse_color + specular_color);
}

vec3 CalcDirectionalLight(vec3 normal, vec3 view_dir, Material material, Texture tex) {

    vec3 diffuse;
    vec3 specular;
    if (tex.texture_on) {
        vec4 color = texture(tex.map, tex_coord*tex.tile_size);
        diffuse = color.rgb;
        specular = color.rgb;
    } else {
        diffuse = material.diffuse;
        specular = material.specular;
    }

    DirectionalLight light = directional_light;
    vec3 light_dir = normalize(-light.direction);
    float diffuse_intensity = max(dot(normal, light_dir), 0.0);
    vec3 diffuse_color = diffuse_intensity * light.diffuse * diffuse;

    vec3 reflect_dir = reflect(-light_dir, normal);
    float specular_intensity = pow(
        max(dot(view_dir, reflect_dir), 0.0), material.shininess);
    vec3 specular_color = specular_intensity * 
        light.specular * specular;

    vec3 final_color = diffuse_color + specular_color;
    return final_color;
}

