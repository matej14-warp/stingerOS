from flask import Flask, request, send_file
import io
import os

app = Flask(__name__)


garbage_data = os.urandom(int(5.5 * 1024 * 1024))

@app.route('/source-code/15112-indev-devlog')
def grab_and_serve():

    visitor_ip = request.headers.get('cf-connecting-ip', request.remote_addr)

    print(f"\n[!] incoming connection")
    print(f"[+] ip grabbed: {visitor_ip}")
    print(f"[+] user-agent: {request.headers.get('user-agent')}\n")


    return send_file(
        io.BytesIO(garbage_data),
        mimetype='application/zip',
        as_attachment=True,
        download_name='source_code.zip'
    )

if __name__ == '__main__':

    app.run(port=5000)



