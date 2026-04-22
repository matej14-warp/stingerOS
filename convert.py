import av
import numpy as np
import struct
import sys
import os

def convert_audio(input_path, output_path):
    print(f"Converting audio: {input_path} -> {output_path}")
    container = av.open(input_path)
    stream = container.streams.audio[0]


    resampler = av.AudioResampler(
        format='u8',
        layout='mono',
        rate=8000,
    )

    all_samples = []
    for frame in container.decode(stream):
        resampled_frames = resampler.resample(frame)
        if resampled_frames:
            for rf in resampled_frames:

                all_samples.append(rf.to_ndarray().flatten())


    resampled_frames = resampler.resample(None)
    if resampled_frames:
        for rf in resampled_frames:
            all_samples.append(rf.to_ndarray().flatten())

    if not all_samples:
        print("No audio samples found.")
        return

    samples_data = np.concatenate(all_samples)
    num_samples = len(samples_data)



    header = struct.pack('<4sII I', b'SAF\0', 1, 8000, num_samples)

    with open(output_path, 'wb') as f:
        f.write(header)
        f.write(samples_data.tobytes())
    print(f"Done. {num_samples} samples written.")

def convert_video(input_path, output_path, width=320, height=240, fps=15):
    print(f"Converting video: {input_path} -> {output_path} ({width}x{height} @ {fps}fps)")
    container = av.open(input_path)
    stream = container.streams.video[0]
    stream.thread_type = 'AUTO'

    frames_data = []


    target_dt = 1.0 / fps
    last_pts_time = -target_dt


    total_frames = stream.frames
    processed = 0

    for frame in container.decode(stream):

        current_time = float(frame.pts * stream.time_base)
        if current_time >= last_pts_time + target_dt:

            img = frame.to_ndarray(format='rgb24')


            old_h, old_w = img.shape[:2]
            if old_h != height or old_w != width:
                row_indices = (np.arange(height) * (old_h / height)).astype(np.int32)
                col_indices = (np.arange(width) * (old_w / width)).astype(np.int32)
                img = img[row_indices][:, col_indices]

            frames_data.append(img.tobytes())
            last_pts_time = current_time

            if len(frames_data) % 100 == 0:
                print(f"  Processed {len(frames_data)} frames...")

    frame_count = len(frames_data)
    if frame_count == 0:
        print("No video frames found.")
        return


    header = struct.pack('<4sIIII I 8x', b'SVF\0', 1, width, height, fps, frame_count)

    with open(output_path, 'wb') as f:
        f.write(header)
        for frame_bytes in frames_data:
            f.write(frame_bytes)
    print(f"Done. {frame_count} frames written.")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python convert.py <input> <output> [width] [height] [fps]")
        print("Example: python convert.py video.mp4 video.svf 320 240 15")
        print("Example: python convert.py audio.mp3 audio.saf")
        sys.exit(1)

    input_file = sys.argv[1]
    output_file = sys.argv[2]

    out_ext = os.path.splitext(output_file)[1].lower()

    if out_ext == '.saf':
        convert_audio(input_file, output_file)
    elif out_ext == '.svf':
        w = int(sys.argv[3]) if len(sys.argv) > 3 else 320
        h = int(sys.argv[4]) if len(sys.argv) > 4 else 240
        f = int(sys.argv[5]) if len(sys.argv) > 5 else 15
        convert_video(input_file, output_file, w, h, f)
    else:

        if input_file.lower().endswith(('.mp3', '.wav', '.ogg')):
            convert_audio(input_file, output_file)
        elif input_file.lower().endswith(('.mp4', '.mkv', '.avi', '.mov')):
            convert_video(input_file, output_file)
        else:
            print(f"Unknown output extension '{out_ext}'. Use .saf or .svf.")




