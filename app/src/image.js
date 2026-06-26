// Baixa uma imagem (URL), redimensiona e converte pra RGB565 (little-endian),
// formato que o display JD9853/LVGL (LV_COLOR_16_SWAP=0) espera.
import Jimp from "jimp";

export async function imageToRgb565(url, size) {
  const resp = await fetch(url);
  const src = Buffer.from(await resp.arrayBuffer());
  const img = await Jimp.read(src);
  img.cover(size, size);  // preenche o quadrado mantendo proporcao (corta sobra)

  const { data, width, height } = img.bitmap; // RGBA
  const out = Buffer.alloc(width * height * 2);
  let o = 0;
  for (let i = 0; i < data.length; i += 4) {
    const r = data[i], g = data[i + 1], b = data[i + 2];
    const rgb565 = ((r & 0xf8) << 8) | ((g & 0xfc) << 3) | (b >> 3);
    out[o++] = rgb565 & 0xff;        // low
    out[o++] = (rgb565 >> 8) & 0xff; // high
  }
  return { buf: out, w: width, h: height };
}
