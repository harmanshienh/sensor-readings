FROM debian:bullseye

RUN apt update && apt install -y \
    python3 python3-venv python3-pip \
    build-essential \
    curl \
    git && \
    apt clean

RUN curl -fsSL https://deb.nodesource.com/setup_20.x | bash - && \
    apt-get install -y nodejs

WORKDIR /smart-mirror

COPY . .

RUN cd sensor_data/face_recognition && \
    python3 -m venv env && \
    . env/bin/activate && \
    pip install -r requirements.txt

RUN cd sensor_data && make

RUN cd MagicMirror && npm install

CMD ["bash", "-c", "cd /smart-mirror/sensor_data/face_recognition && . env/bin/activate && python main.py & cd /smart-mirror/sensor_data && ./uart_receiver & cd /smart-mirror/MagicMirror && npm start"]