import { useTheme } from './useTheme';
import { CadBoundary } from './CadBoundary';
import { ActionEditor } from './ActionEditor';
import { LightingPanel } from './LightingPanel';
import { SettingsPanel } from './SettingsPanel';
import { lazy, Suspense, useEffect, useRef, useState } from 'react';
import {
  Grid2X2,
  SlidersHorizontal,
  Lightbulb,
  Archive,
  Usb,
  Bluetooth,
  ChevronRight,
  Plus,
  Copy,
  Trash2,
  ArrowLeft,
  ArrowRight,
  Download,
  Upload,
  Save,
  RotateCcw,
  Check,
  Activity,
  Monitor,
  Power,
  X,
} from 'lucide-react';
import {
  actionLabel,
  controlName,
  emptyAction,
  stepLabel,
  validate,
  type Action,
  type Profile,
} from './model';
import { UncertainSaveError } from './serial';
import { useDeviceConnection } from './useDeviceConnection';
const CadView = lazy(() => import('./CadView'));
const pages = [
  ['MacroPad', Grid2X2],
  ['Illuminazione', Lightbulb],
  ['Impostazioni', SlidersHorizontal],
  ['Backup', Archive],
] as const;
const clone = <T,>(v: T): T => structuredClone(v);
export default function App() {
  const [page, setPage] = useState('MacroPad'),
    [draft, setDraft] = useState<Profile | null>(null),
    [saved, setSaved] = useState(''),
    [defaults, setDefaults] = useState<Profile | null>(null);
  const [selected, setSelected] = useState(0),
    [layer, setLayer] = useState(0),
    [demo, setDemo] = useState(false),
    [revision, setRevision] = useState(0),
    [busy, setBusy] = useState(false),
    [message, setMessage] = useState(''),
    [error, setError] = useState(''),
    [lastInput, setLastInput] = useState(''),
    [highlight, setHighlight] = useState(-1),
    [sequence, setSequence] = useState(false);
  const timer = useRef<ReturnType<typeof setTimeout> | undefined>(undefined);
  const upload = useRef<HTMLInputElement>(null);
  const { theme, setTheme } = useTheme();
  const [view, setView] = useState<'3d' | 'simple'>('3d'),
    [cadFailed, setCadFailed] = useState(false),
    [pulse, setPulse] = useState({ index: -1, id: 0 });
  const [needsReadback, setNeedsReadback] = useState(false);
  const connection = useDeviceConnection({
    busy,
    onError: setError,
    onInput: (e) => {
      const i =
        e.control === 'key' ? e.value : e.control === 'encoder' ? (e.value > 0 ? 9 : 10) : 11;
      flash(i);
      setLastInput(e.control === 'hold' ? 'Pressione lunga' : controlName(i));
    },
  });
  const { device, connected, testing, setTesting, status, setStatus } = connection;
  async function reread() {
    setBusy(true);
    setError('');
    try {
      const r = await connection.reread();
      setRevision(r.revision);
      setSaved(JSON.stringify(r.profile));
      setNeedsReadback(false);
      setMessage(
        'Profilo riletto. La tua bozza è conservata: verifica le modifiche prima di salvarle.',
      );
    } catch (e) {
      setError((e as Error).message);
    } finally {
      setBusy(false);
    }
  }
  const dirty = !!draft && JSON.stringify(draft) !== saved;
  useEffect(() => {
    fetch('/profilo-iniziale.json')
      .then((r) => r.json())
      .then((p) => {
        validate(p);
        setDefaults(p);
      })
      .catch(() => setError('Profilo iniziale non disponibile. Riavvia Studio.'));
    return () => {
      clearTimeout(timer.current);
    };
  }, []);
  useEffect(() => {
    const before = (e: BeforeUnloadEvent) => {
      if (dirty) {
        e.preventDefault();
        e.returnValue = '';
      }
    };
    window.addEventListener('beforeunload', before);
    return () => window.removeEventListener('beforeunload', before);
  }, [dirty]);
  function update(fn: (p: Profile) => void) {
    if (!draft) return;
    const p = clone(draft);
    fn(p);
    setDraft(p);
    setMessage('');
  }
  function action(a: Action) {
    update((p) => {
      p.layers[layer].actions[selected] = a;
    });
  }
  function select(i: number) {
    setSelected(i);
    setSequence(false);
  }
  function flash(i: number) {
    setPulse((p) => ({ index: i, id: p.id + 1 }));
    setHighlight(i);
    clearTimeout(timer.current);
    timer.current = setTimeout(() => setHighlight(-1), 700);
  }
  async function connect() {
    if (
      dirty &&
      !confirm('Collegandoti verrà letto il profilo del dispositivo. Scartare la bozza attuale?')
    )
      return;
    setBusy(true);
    setError('');
    try {
      const r = await connection.open();
      setDraft(r.profile);
      setSaved(JSON.stringify(r.profile));
      setRevision(r.revision);
      setNeedsReadback(false);
      setDemo(false);
      setLayer(0);
      setMessage('Profilo letto dal MacroPad.');
    } catch (e) {
      setError(
        (e as Error).name === 'NotFoundError'
          ? 'Nessuna porta selezionata.'
          : `Connessione non riuscita: ${(e as Error).message}. Se la porta è occupata, chiudi il monitor seriale.`,
      );
      await connection.disconnect();
    } finally {
      setBusy(false);
    }
  }
  async function save() {
    if (!draft || needsReadback) return;
    setError('');
    try {
      validate(draft);
      if (demo) {
        setSaved(JSON.stringify(draft));
        setMessage('Bozza della demo aggiornata. Nessun dispositivo modificato.');
        return;
      }
      setBusy(true);
      if (testing) {
        await device.current!.test(false);
        setTesting(false);
      }
      const r = await device.current!.save(draft, revision);
      setRevision(r.revision);
      setSaved(JSON.stringify(draft));
      setMessage('Configurazione salvata e verificata sul MacroPad.');
      setStatus(await device.current!.status());
    } catch (e) {
      if (e instanceof UncertainSaveError) setNeedsReadback(true);
      setError((e as Error).message);
    } finally {
      setBusy(false);
    }
  }
  async function test() {
    setLastInput('');
    try {
      if (demo) {
        setTesting(!testing);
        return;
      }
      setTesting(await device.current!.test(!testing));
    } catch (e) {
      setError((e as Error).message);
    }
  }
  function demoStart() {
    if (!defaults) return;
    if (dirty && !confirm('Scartare la bozza e aprire la demo?')) return;
    setDemo(true);
    setDraft(clone(defaults));
    setSaved(JSON.stringify(defaults));
    setLayer(0);
    setSelected(0);
    setMessage('Modalità demo: nessun dispositivo collegato.');
    setError('');
  }
  function exportProfile() {
    try {
      validate(draft);
      const url = URL.createObjectURL(
        new Blob([JSON.stringify(draft, null, 2)], { type: 'application/json' }),
      );
      const a = document.createElement('a');
      a.href = url;
      a.download = 'nove-profilo.json';
      a.click();
      URL.revokeObjectURL(url);
    } catch (e) {
      setError((e as Error).message);
    }
  }
  async function importProfile(file?: File) {
    if (!file) return;
    try {
      if (file.size > 32768) throw new Error('File troppo grande: massimo 32 KiB.');
      const p = JSON.parse(await file.text());
      validate(p);
      if (dirty && !confirm('Sostituire la bozza con il profilo importato?')) return;
      setDraft(p);
      setLayer(0);
      setSelected(0);
      setMessage('Profilo importato nella bozza. Salvalo per applicarlo.');
      setError('');
    } catch (e) {
      setError((e as Error).message);
    } finally {
      if (upload.current) upload.current.value = '';
    }
  }
  const current = draft?.layers[layer],
    a = current?.actions[selected];
  return (
    <div className="shell">
      <aside className="sidebar">
        <div className="brand">
          <span className="brand-mark">9</span>
          <div>
            NOVE <b>Studio</b>
            <small>BY 99BIT / MACROPAD</small>
          </div>
        </div>
        <div className="workspace-label">IL TUO DISPOSITIVO</div>
        <nav>
          {pages.map(([name, Icon]) => (
            <button
              key={name}
              className={page === name ? 'nav-item active' : 'nav-item'}
              onClick={() => setPage(name)}
            >
              <Icon size={19} />
              {name}
              {page === name && <ChevronRight size={15} />}
            </button>
          ))}
        </nav>
        <div className="sidebar-bottom">
          <span className="version">STUDIO 1.1 · CANDIDATE</span>
          <p>
            Progettato per il tuo flusso.
            <br />
            Configurato sul tuo Mac.
          </p>
          <div className="local">
            <span />
            Locale · nessun account
          </div>
        </div>
      </aside>
      <main>
        <header className="topbar">
          <div className="breadcrumb">
            Dispositivi <ChevronRight size={14} />
            <strong>NOVE MacroPad</strong>
          </div>
          <div className="connections">
            <select
              className="theme-select"
              aria-label="Tema"
              value={theme}
              onChange={(e) => setTheme(e.target.value)}
            >
              <option value="system">Sistema</option>
              <option value="light">Chiaro</option>
              <option value="dark">Scuro</option>
            </select>
            <span className={'badge ' + (connected ? 'good' : '')}>
              <Usb size={14} />
              USB {connected ? 'collegata' : 'scollegata'}
            </span>
            <span className={'badge ' + (status?.ready ? 'good' : '')}>
              <Bluetooth size={14} />
              BLE{' '}
              {status
                ? status.ready
                  ? 'pronto'
                  : status.connected
                    ? 'in connessione'
                    : 'non connesso'
                : 'non rilevato'}
            </span>
            <button
              className="button"
              disabled={busy}
              onClick={
                connected
                  ? async () => {
                      await connection.disconnect();
                    }
                  : connect
              }
            >
              {connected ? 'Disconnetti' : 'Connetti MacroPad'}
            </button>
          </div>
        </header>
        <div className="content">
          <div className="page-title">
            <div>
              <span className="eyebrow">99BIT / CONTROLLO LOCALE</span>
              <h1>{page}</h1>
              <p>
                {page === 'MacroPad'
                  ? 'Ogni gesto, esattamente come lo vuoi.'
                  : page === 'Illuminazione'
                    ? 'Un colore per riconoscere il tuo spazio di lavoro.'
                    : page === 'Impostazioni'
                      ? 'Il comportamento giusto, anche a Studio chiuso.'
                      : 'I tuoi profili, sempre con te.'}
              </p>
            </div>
            {draft && (
              <span className={'draft-state ' + (dirty ? 'unsaved' : '')}>
                <span />
                {dirty ? 'Modifiche non salvate' : 'Profilo sincronizzato'}
                {demo ? ' · DEMO' : ''}
              </span>
            )}
          </div>
          {demo && (
            <div className="notice demo">
              <Monitor size={17} />
              <strong>Modalità demo</strong> Stai esplorando un dispositivo simulato. Nessuna
              modifica viene inviata all’hardware.
            </div>
          )}
          {error && (
            <div role="alert" className="notice error">
              {error}
              <button aria-label="Chiudi errore" onClick={() => setError('')}>
                <X size={16} />
              </button>
            </div>
          )}
          {needsReadback && (
            <div className="notice error" role="status">
              <span>Salvataggio da verificare. La bozza è conservata.</span>
              <button className="button" disabled={busy || !connected} onClick={reread}>
                Rileggi dispositivo
              </button>
            </div>
          )}
          {message && (
            <div role="status" className="notice success">
              <Check size={16} />
              {message}
            </div>
          )}
          {!draft ? (
            <section className="welcome">
              <div className="welcome-copy">
                <span className="eyebrow">BENVENUTO IN NOVE STUDIO</span>
                <h2>
                  Il tuo MacroPad.
                  <br />
                  Il tuo modo di lavorare.
                </h2>
                <p>
                  Collega il dispositivo via USB per personalizzare tasti, manopola e illuminazione.
                  Le impostazioni rimangono sul MacroPad.
                </p>
                <button className="button primary" disabled={busy} onClick={connect}>
                  <Usb size={17} />
                  {busy ? 'Connessione…' : 'Connetti MacroPad'}
                </button>
                <button className="text-button" disabled={!defaults || busy} onClick={demoStart}>
                  Esplora la demo <ArrowRight size={16} />
                </button>
                <ol>
                  <li>Collega la USB al Mac</li>
                  <li>Seleziona la porta del MacroPad</li>
                  <li>Personalizza e salva</li>
                </ol>
              </div>
              <div className="welcome-device">
                {!cadFailed ? (
                  <CadBoundary
                    onError={() => {
                      setCadFailed(true);
                      setView('simple');
                    }}
                  >
                    <Suspense fallback={<span>Caricamento modello…</span>}>
                      <CadView
                        color="#38c9db"
                        selected={-1}
                        pulse={pulse}
                        disabled={true}
                        onSelect={() => {}}
                        onError={() => setCadFailed(true)}
                      />
                    </Suspense>
                  </CadBoundary>
                ) : (
                  <p>NOVE · 9 tasti e una manopola</p>
                )}
                <span className="caption">9 TASTI · 1 MANOPOLA · LE TUE IDEE</span>
              </div>
            </section>
          ) : (
            <fieldset disabled={busy} className="work-area">
              {page === 'MacroPad' && current && a && (
                <>
                  <div className="layer-toolbar">
                    <div className="tabs" role="tablist" aria-label="Layer">
                      {draft.layers.map((l, i) => (
                        <button
                          role="tab"
                          aria-selected={layer === i}
                          className={layer === i ? 'tab selected' : 'tab'}
                          key={i}
                          onClick={() => {
                            setLayer(i);
                            select(0);
                          }}
                        >
                          <span style={{ background: l.color }} />
                          {l.name || 'Senza nome'}
                        </button>
                      ))}
                    </div>
                    <button
                      className="button"
                      disabled={draft.layers.length >= 8}
                      onClick={() => {
                        update((p) =>
                          p.layers.push({
                            name: `LAYER ${p.layers.length + 1}`,
                            color: '#007fff',
                            actions: Array.from({ length: 12 }, emptyAction),
                          }),
                        );
                        setLayer(draft.layers.length);
                        select(0);
                      }}
                    >
                      <Plus size={16} />
                      Nuovo layer
                    </button>
                  </div>
                  <div className="editor-layout">
                    <section className="device-panel">
                      <div className="panel-heading">
                        <div>
                          <h2>Layout comandi</h2>
                          <p>Seleziona un tasto per personalizzarlo</p>
                        </div>
                        <button
                          className={'button ' + (testing ? 'primary' : '')}
                          disabled={!connected && !demo}
                          onClick={test}
                        >
                          <Activity size={15} />
                          {testing ? 'Termina prova' : 'Prova comandi'}
                        </button>
                      </div>
                      {testing && (
                        <div className="test-banner">
                          {demo
                            ? 'Clicca i tasti per simulare la prova.'
                            : 'Premi i comandi fisici: le macro sono temporaneamente disattivate.'}
                        </div>
                      )}
                      {testing && !demo && (
                        <p className="test-input-status" role="status">
                          {lastInput
                            ? `Ultimo comando ricevuto: ${lastInput}`
                            : 'In attesa di un comando dal MacroPad…'}
                        </p>
                      )}
                      <div className="view-switch" role="group" aria-label="Vista del MacroPad">
                        <button
                          className={view === '3d' ? 'selected' : ''}
                          disabled={cadFailed}
                          onClick={() => setView('3d')}
                        >
                          Modello 3D
                        </button>
                        <button
                          className={view === 'simple' ? 'selected' : ''}
                          onClick={() => setView('simple')}
                        >
                          Vista semplice
                        </button>
                      </div>
                      {cadFailed && (
                        <p className="hint">
                          Vista 3D non disponibile. Puoi configurare tutti i comandi nella vista
                          semplice.
                        </p>
                      )}
                      {view === '3d' ? (
                        <>
                          <CadBoundary
                            onError={() => {
                              setCadFailed(true);
                              setView('simple');
                            }}
                          >
                            <Suspense
                              fallback={
                                <div className="cad-wrap cad-loading">Caricamento vista 3D…</div>
                              }
                            >
                              <CadView
                                color={current.color}
                                selected={selected}
                                pulse={pulse}
                                disabled={busy}
                                onSelect={(i) => {
                                  select(i);
                                  if (demo && testing) flash(i);
                                }}
                                onError={() => {
                                  setCadFailed(true);
                                  setView('simple');
                                }}
                              />
                            </Suspense>
                          </CadBoundary>
                          <div className="cad-key-controls" aria-label="Tasti del MacroPad">
                            {current.actions.slice(0, 9).map((act, i) => (
                              <button
                                key={i}
                                aria-label={`Tasto ${i + 1}: ${actionLabel(act)}`}
                                aria-pressed={selected === i}
                                className={selected === i ? 'selected' : ''}
                                onClick={() => {
                                  select(i);
                                  if (demo && testing) flash(i);
                                }}
                              >
                                <small>{i + 1}</small>
                                <span>{actionLabel(act)}</span>
                              </button>
                            ))}
                          </div>
                        </>
                      ) : (
                        <div className="device-stage">
                          <div
                            className="hardware"
                            style={{ '--layer-color': current.color } as React.CSSProperties}
                          >
                            <div className="hardware-top">
                              <div className="oled">
                                <small>
                                  {demo ? 'DEMO' : status?.ready ? 'BLE PRONTO' : 'NOVE MACROPAD'}
                                </small>
                                <strong>{current.name}</strong>
                                <span>Anteprima del layer</span>
                              </div>
                              <button
                                className={
                                  'knob ' +
                                  (selected >= 9 ? 'chosen' : '') +
                                  ' ' +
                                  (highlight >= 9 ? 'lit' : '')
                                }
                                aria-label="Configura manopola"
                                onClick={() => {
                                  select(11);
                                  if (demo && testing) flash(11);
                                }}
                              >
                                <span />
                              </button>
                            </div>
                            <div className="key-grid">
                              {current.actions.slice(0, 9).map((act, i) => (
                                <button
                                  key={i}
                                  aria-label={`Tasto ${i + 1}: ${actionLabel(act)}`}
                                  aria-pressed={selected === i}
                                  className={
                                    'key ' +
                                    (selected === i ? 'chosen' : '') +
                                    ' ' +
                                    (highlight === i ? 'lit' : '')
                                  }
                                  onClick={() => {
                                    select(i);
                                    if (demo && testing) flash(i);
                                  }}
                                >
                                  <small>{String(i + 1).padStart(2, '0')}</small>
                                  <span>
                                    {act.steps.length === 1
                                      ? stepLabel(act.steps[0])
                                      : act.steps.length
                                        ? '•••'
                                        : '—'}
                                  </span>
                                  <em>{actionLabel(act)}</em>
                                </button>
                              ))}
                            </div>
                            <div className="light-strip" />
                          </div>
                        </div>
                      )}
                      <div className="encoder-controls">
                        {[9, 10, 11].map((i) => (
                          <button
                            key={i}
                            className={
                              (selected === i ? 'selected' : '') +
                              (highlight === i ? ' input-lit' : '')
                            }
                            onClick={() => {
                              select(i);
                              if (demo && testing) flash(i);
                            }}
                          >
                            {i === 9 ? (
                              <RotateCcw style={{ transform: 'scaleX(-1)' }} size={16} />
                            ) : i === 10 ? (
                              <RotateCcw size={16} />
                            ) : (
                              <Power size={16} />
                            )}
                            <span>{controlName(i)}</span>
                          </button>
                        ))}
                      </div>
                      <div className="device-footnote">
                        <span>Pressione lunga: cambia layer</span>
                        <span>
                          Layer attivo:{' '}
                          {demo
                            ? 'simulato'
                            : status
                              ? draft.layers[status.layer]?.name || '—'
                              : 'non rilevato'}
                        </span>
                      </div>
                    </section>
                    <ActionEditor
                      a={a}
                      selected={selected}
                      sequence={sequence}
                      setSequence={setSequence}
                      action={action}
                    />
                  </div>
                  <section className="layer-settings">
                    <label>
                      Nome del layer
                      <input
                        value={current.name}
                        onChange={(e) =>
                          update((p) => {
                            p.layers[layer].name = e.target.value;
                          })
                        }
                      />
                    </label>
                    <div className="row">
                      <button
                        className="button"
                        disabled={layer === 0}
                        onClick={() => {
                          update((p) => {
                            [p.layers[layer - 1], p.layers[layer]] = [
                              p.layers[layer],
                              p.layers[layer - 1],
                            ];
                          });
                          setLayer(layer - 1);
                        }}
                      >
                        <ArrowLeft size={16} />
                        Sposta
                      </button>
                      <button
                        className="button"
                        disabled={layer === draft.layers.length - 1}
                        onClick={() => {
                          update((p) => {
                            [p.layers[layer + 1], p.layers[layer]] = [
                              p.layers[layer],
                              p.layers[layer + 1],
                            ];
                          });
                          setLayer(layer + 1);
                        }}
                        aria-label="Sposta layer a destra"
                      >
                        <ArrowRight size={16} />
                      </button>
                      <button
                        className="button"
                        disabled={draft.layers.length >= 8}
                        onClick={() => {
                          update((p) => {
                            const l = clone(p.layers[layer]);
                            l.name = 'COPIA';
                            p.layers.push(l);
                          });
                          setLayer(draft.layers.length);
                        }}
                      >
                        <Copy size={15} />
                        Duplica
                      </button>
                      <button
                        className="button danger"
                        disabled={draft.layers.length === 1}
                        onClick={() => {
                          if (confirm(`Eliminare il layer ${current.name} dalla bozza?`)) {
                            update((p) => p.layers.splice(layer, 1));
                            setLayer(Math.max(0, layer - 1));
                          }
                        }}
                      >
                        <Trash2 size={15} />
                        Elimina
                      </button>
                    </div>
                  </section>
                </>
              )}
              {page === 'Illuminazione' && <LightingPanel draft={draft} update={update} />}
              {page === 'Impostazioni' && <SettingsPanel draft={draft} update={update} />}
              {page === 'Backup' && (
                <section className="settings-panel">
                  <h2>Profili e ripristino</h2>
                  <div className="backup-row">
                    <Download />
                    <div>
                      <h3>Esporta il profilo</h3>
                      <p>Salva tutti i layer e le impostazioni della bozza in un file JSON.</p>
                    </div>
                    <button className="button" onClick={exportProfile}>
                      Esporta
                    </button>
                  </div>
                  <div className="backup-row">
                    <Upload />
                    <div>
                      <h3>Importa un profilo</h3>
                      <p>Controlliamo il file prima di sostituire la bozza. Massimo 32 KiB.</p>
                    </div>
                    <button className="button" onClick={() => upload.current?.click()}>
                      Importa
                    </button>
                    <input
                      hidden
                      ref={upload}
                      type="file"
                      accept=".json,application/json"
                      onChange={(e) => void importProfile(e.target.files?.[0])}
                    />
                  </div>
                  <div className="backup-row">
                    <RotateCcw />
                    <div>
                      <h3>Riparti dalle impostazioni iniziali</h3>
                      <p>Ripristina MAC, MEDIA e WORK. L’abbinamento Bluetooth viene conservato.</p>
                    </div>
                    <button
                      className="button"
                      disabled={!defaults}
                      onClick={() => {
                        if (confirm('Sostituire la bozza con il profilo iniziale?')) {
                          setDraft(clone(defaults!));
                          setLayer(0);
                          setMessage('Profilo iniziale nella bozza. Salva per applicarlo.');
                        }
                      }}
                    >
                      Ripristina
                    </button>
                  </div>
                </section>
              )}
            </fieldset>
          )}
        </div>
        {draft && (
          <footer className="savebar">
            <div>
              <span className={'status-dot ' + (dirty ? 'amber' : '')} />
              <strong>{dirty ? 'Hai modifiche da salvare' : 'Nessuna modifica in sospeso'}</strong>
              <small>
                {demo
                  ? 'Demo locale · nessuna scrittura USB'
                  : connected
                    ? 'Le impostazioni restano nel dispositivo'
                    : 'USB scollegata · bozza conservata'}
              </small>
            </div>
            <button
              className="button"
              disabled={!dirty || busy}
              onClick={() => {
                if (confirm('Scartare tutte le modifiche non salvate?')) {
                  setDraft(JSON.parse(saved));
                  setLayer(0);
                  setMessage('Bozza ripristinata.');
                }
              }}
            >
              Annulla modifiche
            </button>
            <button
              className="button primary"
              disabled={needsReadback || busy || !dirty || (!connected && !demo)}
              onClick={save}
            >
              <Save size={16} />
              {busy ? 'Attendi…' : demo ? 'Salva nella demo' : 'Salva sul MacroPad'}
            </button>
          </footer>
        )}
      </main>
    </div>
  );
}
