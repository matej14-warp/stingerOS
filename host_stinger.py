from flask import Flask, Response, send_from_directory
import os

app = Flask(__name__)

FILE_DIR = os.path.abspath(".")


SITEMAP_XML = """<?xml version="1.0" encoding="UTF-8"?>
<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">
  <url>
    <loc>https://voided.uk/</loc>
    <lastmod>2026-04-21</lastmod>
    <changefreq>monthly</changefreq>
    <priority>1.0</priority>
  </url>
</urlset>"""

@app.route('/')
def home():
    return send_from_directory(FILE_DIR, 'root.html')

@app.route('/sitemap.xml')
def sitemap():

    return Response(SITEMAP_XML, mimetype='application/xml')

@app.route('/<path:filename>')
def serve_files(filename):

    allowed_exts = ('.iso', '.zip', '.ico', '.exe')
    if any(filename.lower().endswith(ext) for ext in allowed_exts):

        return send_from_directory(FILE_DIR, filename, as_attachment=True)
    return "not authorized", 404

if __name__ == '__main__':
    app.run(port=6767)

