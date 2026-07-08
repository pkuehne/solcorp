#include "modules/lua/texture_data.h"

std::vector<TextureDef> parseTextureData(const ModValue &root) {
  std::vector<TextureDef> textures;

  root.forEachEntry([&](const std::string &name, const ModValue &value) {
    if (value.isTable()) {
      TextureDef def;
      def.name = name;
      def.file = value.getString("file").value_or("");
      textures.push_back(def);
    }
  });

  return textures;
}
