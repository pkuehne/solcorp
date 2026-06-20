#include "modules/lua/texture_data.h"

std::vector<TextureDef> parseTextureData(const LuaTableView &root) {
  std::vector<TextureDef> textures;

  root.forEachEntry([&](const std::string &name, const LuaValue &value) {
    if (auto texture = value.asTable()) {
      TextureDef def;
      def.name = name;
      def.file = texture->getString("file").value_or("");
      textures.push_back(def);
    }
  });

  return textures;
}
