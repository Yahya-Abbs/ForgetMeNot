import React, { useState, useEffect, useRef } from 'react';
import {
  LineChart,
  Line,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  ResponsiveContainer,
} from 'recharts';
import { Heart, Check, X } from 'lucide-react';

function App() {
  // States for graph data
  const [data1, setData1] = useState([]); // BPM data for Heart Rate graph
  const [data2, setData2] = useState([]); // SpO₂ data for Oxygen Saturation graph

  // State for fall detection
  const [fallDetected, setFallDetected] = useState(false);

  // Audio ref for the alarm sound
  const alarmAudioRef = useRef(null);

  const ESP32_IP = "http://192.168.246.4";
  const FETCH_INTERVAL = 1000; // Poll every 5 seconds

  useEffect(() => {
    // Function to fetch data from the ESP32
    const fetchData = () => {
      fetch(`${ESP32_IP}/data`)
        .then((res) => res.json())
        .then((data) => {
          console.log("Received Data:", data);
       
          setData1((prev) =>
            [...prev, { time: new Date().toLocaleTimeString(), value: data.bpm }].slice(-10)
          );
          setData2((prev) =>
            [...prev, { time: new Date().toLocaleTimeString(), value: data.spo2 }].slice(-10)
          );

          setFallDetected(prev => prev || (data.fall === 1));
        })
        .catch((err) => console.error("Error fetching data:", err));
    };

    // Fetch data immediately and then at regular intervals
    fetchData();
    const interval = setInterval(fetchData, FETCH_INTERVAL);
    return () => clearInterval(interval);
  }, [ESP32_IP]);

  // Play or stop the alarm sound based on fall detection
  useEffect(() => {
    if (fallDetected) {
      alarmAudioRef.current
        .play()
        .catch((e) => console.error("Audio play error:", e));
    } else if (alarmAudioRef.current) {
      alarmAudioRef.current.pause();
      alarmAudioRef.current.currentTime = 0;
    }
  }, [fallDetected]);

  // Optional: custom dot renderer for the graphs
  const renderDot = (props) => {
    const { cx, cy, index, dataKey } = props;
    if (index === 0) return null;
    return (
      <circle
        key={`dot-${dataKey}-${index}`}
        cx={cx}
        cy={cy}
        r={7}
        fill={props.stroke}
        stroke={props.stroke}
      />
    );
  };

  return (
    <div className="min-h-screen bg-gradient-to-b from-gray-900 via-black to-gray-900 p-20 text-white">
      {/* Hidden audio element for the alarm sound */}
      <audio
        ref={alarmAudioRef}
        src="https://actions.google.com/sounds/v1/alarms/alarm_clock.ogg"
        loop
      />
      <div className="max-w-7xl mx-auto space-y-16">
        <h1 className="text-6xl font-bold text-center mb-12 flex justify-center items-center gap-4">
          <Heart className="w-16 h-16 text-red-500" />
          Patient Status
        </h1>

        <div className="grid grid-cols-3 gap-16">
          <div className="col-span-2 space-y-16">
            <div className="bg-gray-900 border border-gray-700 rounded-2xl p-10 shadow-xl">
              <div className="flex items-center justify-between mb-6">
                <h2 className="text-3xl font-semibold">Heart Rate</h2>
                <span className="text-lg text-gray-300">Real-time BPM</span>
              </div>
              <div className="h-[600px]">
                <ResponsiveContainer width="100%" height="100%">
                  <LineChart data={data1}>
                    <CartesianGrid strokeDasharray="3 3" stroke="#444" />
                    <XAxis dataKey="time" stroke="#fff" />
                    <YAxis stroke="#fff" domain={[0, 200]} />
                    <Tooltip contentStyle={{ backgroundColor: '#1a1a1a', border: '1px solid #333', color: '#fff' }} />
                    <Line
                      type="monotone"
                      dataKey="value"
                      stroke="#ff4d4d"
                      strokeWidth={4}
                      dot={renderDot}
                      activeDot={{ r: 10, fill: '#ff4d4d', stroke: '#fff' }}
                    />
                  </LineChart>
                </ResponsiveContainer>
              </div>
            </div>

            {/* Oxygen Saturation Graph */}
            <div className="bg-gray-900 border border-gray-700 rounded-2xl p-10 shadow-xl">
              <div className="flex items-center justify-between mb-6">
                <h2 className="text-3xl font-semibold">Oxygen Saturation</h2>
                <span className="text-lg text-gray-300">Real-time SpO₂ %</span>
              </div>
              <div className="h-[600px]">
                <ResponsiveContainer width="100%" height="100%">
                  <LineChart data={data2}>
                    <CartesianGrid strokeDasharray="3 3" stroke="#444" />
                    <XAxis dataKey="time" stroke="#fff" />
                    <YAxis stroke="#fff" domain={[0, 100]} />
                    <Tooltip contentStyle={{ backgroundColor: '#1a1a1a', border: '1px solid #333', color: '#fff' }} />
                    <Line
                      type="monotone"
                      dataKey="value"
                      stroke="#ffa31a"
                      strokeWidth={4}
                      dot={renderDot}
                      activeDot={{ r: 10, fill: '#ffa31a', stroke: '#fff' }}
                    />
                  </LineChart>
                </ResponsiveContainer>
              </div>
            </div>
          </div>

          {/* Right Column: Fall Detection Alarm */}
          <div className="bg-gray-900 border border-gray-700 rounded-2xl p-10 shadow-xl h-fit flex flex-col items-center justify-center">
            <h2 className="text-3xl font-semibold mb-10">Fall Detection</h2>
            <div className="relative w-56 h-56 mb-10">
              {fallDetected ? (
                <X className="text-red-500 w-56 h-56" />
              ) : (
                <Check className="text-green-500 w-56 h-56" />
              )}
            </div>
            {fallDetected ? (
              <button
                onClick={() => setFallDetected(false)}
                className="mt-4 px-8 py-4 bg-red-600 text-white rounded-2xl hover:bg-red-700 transition text-2xl"
              >
                Dismiss Alarm
              </button>
            ) : (
              <div className="mt-2 text-gray-300 text-2xl">No fall detected</div>
            )}
          </div>
        </div>
      </div>
    </div>
  );
}

export default App;
