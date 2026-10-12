import { useEffect, useRef, useState } from 'react';
import * as T from 'three';
import { GLTFLoader } from 'three/addons/loaders/GLTFLoader.js';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';

type Props = {
  color: string;
  selected: number;
  pulse: { index: number; id: number };
  disabled: boolean;
  onSelect: (i: number) => void;
  onError: () => void;
};
export default function CadView(props: Props) {
  const host = useRef<HTMLDivElement>(null),
    latest = useRef(props),
    refresh = useRef(() => {}),
    reset = useRef(() => {});
  latest.current = props;
  const [loading, setLoading] = useState(true);
  useEffect(() => {
    const el = host.current!;
    let stopped = false,
      frame = 0;
    const resources: T.Material[] = [];
    let model: T.Object3D | undefined;
    let renderer: T.WebGLRenderer;
    try {
      renderer = new T.WebGLRenderer({ antialias: true, alpha: true });
    } catch {
      latest.current.onError();
      return;
    }
    renderer.setPixelRatio(Math.min(devicePixelRatio, 2));
    renderer.setClearColor(0, 0);
    renderer.outputColorSpace = T.SRGBColorSpace;
    el.appendChild(renderer.domElement);
    renderer.domElement.setAttribute(
      'aria-label',
      'Modello CAD V29: trascina per ruotare; usa i pulsanti sottostanti per selezionare i comandi.',
    );
    const contextLost = (event: Event) => {
      event.preventDefault();
      if (!stopped) latest.current.onError();
    };
    renderer.domElement.addEventListener('webglcontextlost', contextLost);
    const scene = new T.Scene(),
      camera = new T.PerspectiveCamera(35, 1, 1, 2000);
    camera.up.set(0, 0, 1);
    const controls = new OrbitControls(camera, renderer.domElement);
    controls.enablePan = false;
    controls.enableZoom = false;
    controls.minPolarAngle = 0.15;
    controls.maxPolarAngle = Math.PI * 0.85;
    const target = new T.Vector3(44, 52, 26);
    reset.current = () => {
      camera.position.copy(target).add(new T.Vector3(145, -210, 230));
      controls.target.copy(target);
      controls.update();
      draw();
    };
    scene.add(new T.HemisphereLight(0xffffff, 0x596779, 3));
    const light = new T.DirectionalLight(0xffffff, 3);
    light.position.set(-100, -80, 250);
    scene.add(light);
    const meshes: T.Mesh[] = [];
    const axis = new T.Vector3(0, -Math.sin(Math.PI / 12), Math.cos(Math.PI / 12));
    let seen = -1,
      start = 0,
      active = -1;
    const reduced = matchMedia('(prefers-reduced-motion: reduce)');
    function draw() {
      if (stopped) return;
      cancelAnimationFrame(frame);
      const p = latest.current;
      if (p.pulse.id !== seen) {
        seen = p.pulse.id;
        start = performance.now();
        active = p.pulse.index;
      }
      const progress = (performance.now() - start) / 450,
        amount = reduced.matches ? 0 : Math.sin(Math.min(1, progress) * Math.PI);
      for (const m of meshes) {
        const mat = m.material as T.MeshStandardMaterial;
        const id = m.userData.control as number | undefined;
        mat.emissive.set(
          id !== undefined && (id === p.selected || (id === 11 && p.selected >= 9))
            ? 0x145d92
            : 0x000000,
        );
        mat.emissiveIntensity = 0.28;
        if (m.name === 'cad-39' || m.name === 'cad-37') {
          mat.color.set(p.color);
          mat.emissive.set(p.color);
          mat.emissiveIntensity = 0.4;
        }
        m.position.copy(m.userData.rest);
        m.quaternion.identity();
        if (id !== undefined && (id === active || (id === 11 && active >= 9))) {
          if (active === 9 || active === 10)
            m.quaternion.setFromAxisAngle(axis, amount * (active === 9 ? -1 : 1) * 0.26);
          else m.position.addScaledVector(axis, -amount * 0.9);
        }
      }
      renderer.render(scene, camera);
      if (progress < 1 && !reduced.matches) frame = requestAnimationFrame(draw);
    }
    refresh.current = draw;
    controls.addEventListener('change', draw);
    const resize = new ResizeObserver(() => {
      camera.aspect = el.clientWidth / Math.max(1, el.clientHeight);
      camera.updateProjectionMatrix();
      renderer.setSize(el.clientWidth, el.clientHeight);
      draw();
    });
    resize.observe(el);
    reset.current();
    const abort = new AbortController();
    fetch('/models/nove-v29.glb', { signal: abort.signal })
      .then((r) => {
        if (!r.ok) throw Error('CAD unavailable');
        return r.arrayBuffer();
      })
      .then((data) => new GLTFLoader().parseAsync(data, ''))
      .then((g) => {
        if (stopped) {
          g.scene.traverse((o) => {
            if (o instanceof T.Mesh) {
              o.geometry.dispose();
              const ms = Array.isArray(o.material) ? o.material : [o.material];
              ms.forEach((m) => m.dispose());
            }
          });
          return;
        }
        model = g.scene;
        model.traverse((o) => {
          if (!(o instanceof T.Mesh)) return;
          const old = Array.isArray(o.material) ? o.material : [o.material];
          old.forEach((m) => m.dispose());
          const id = o.name.startsWith('key-')
            ? Number(o.name.slice(4))
            : o.name === 'knob'
              ? 11
              : undefined;
          const printed = ['cad-25', 'cad-27', 'cad-38', 'cad-40'].includes(o.name);
          const milky = id !== undefined || o.name === 'cad-39';
          const pcb = ['cad-12', 'cad-13', 'cad-26', 'cad-33'].includes(o.name);
          const mat = new T.MeshStandardMaterial({
            color: milky
              ? 0xe8e9e3
              : printed
                ? 0x17191c
                : pcb
                  ? 0x204d39
                  : o.name === 'cad-10'
                    ? 0x121b20
                    : 0x9d9fa3,
            roughness: milky ? 0.65 : printed ? 0.9 : 0.4,
            metalness: printed || milky || pcb ? 0 : 0.55,
          });
          resources.push(mat);
          o.material = mat;
          const center =
            o.geometry.boundingBox?.getCenter(new T.Vector3()) ??
            new T.Box3()
              .setFromBufferAttribute(o.geometry.attributes.position as T.BufferAttribute)
              .getCenter(new T.Vector3());
          o.geometry.translate(-center.x, -center.y, -center.z);
          o.position.add(center);
          o.userData.rest = o.position.clone();
          o.userData.control = id;
          meshes.push(o);
        });
        scene.add(model);
        setLoading(false);
        draw();
      })
      .catch((e) => {
        if (!stopped && e.name !== 'AbortError') latest.current.onError();
      });
    let down = { x: 0, y: 0 };
    const pointerDown = (e: PointerEvent) => {
      down = { x: e.clientX, y: e.clientY };
    };
    const pointerUp = (e: PointerEvent) => {
      if (latest.current.disabled || Math.hypot(e.clientX - down.x, e.clientY - down.y) > 5) return;
      const rect = el.getBoundingClientRect();
      const ray = new T.Raycaster();
      ray.setFromCamera(
        new T.Vector2(
          ((e.clientX - rect.left) / rect.width) * 2 - 1,
          (-(e.clientY - rect.top) / rect.height) * 2 + 1,
        ),
        camera,
      );
      const hit = ray.intersectObjects(meshes)[0];
      if (hit && hit.object.userData.control !== undefined)
        latest.current.onSelect(hit.object.userData.control);
    };
    renderer.domElement.addEventListener('pointerdown', pointerDown);
    renderer.domElement.addEventListener('pointerup', pointerUp);
    return () => {
      stopped = true;
      abort.abort();
      cancelAnimationFrame(frame);
      resize.disconnect();
      controls.dispose();
      renderer.domElement.removeEventListener('pointerdown', pointerDown);
      renderer.domElement.removeEventListener('pointerup', pointerUp);
      renderer.domElement.removeEventListener('webglcontextlost', contextLost);
      meshes.forEach((m) => m.geometry.dispose());
      resources.forEach((m) => m.dispose());
      renderer.dispose();
      renderer.domElement.remove();
      refresh.current = () => {};
    };
  }, []);
  useEffect(() => refresh.current(), [props.color, props.selected, props.pulse]);
  return (
    <div className="cad-wrap">
      <div ref={host} className="cad-canvas" />
      {loading && (
        <span className="cad-loading" role="status">
          Caricamento CAD…
        </span>
      )}
      <button type="button" className="button cad-reset" onClick={() => reset.current()}>
        Ripristina vista
      </button>
      <span className="cad-caption">CAD V29 · trascina per ruotare</span>
    </div>
  );
}
