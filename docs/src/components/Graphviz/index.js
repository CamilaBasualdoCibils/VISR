import React, { useEffect, useState } from 'react';
import styles from './styles.module.css';

export default function Graphviz({ children, label = 'Graphviz diagram' }) {
  const [svg, setSvg] = useState('');
  const [error, setError] = useState('');
  useEffect(() => {
    let active = true;
    import('@viz-js/viz').then(({ instance }) => instance()).then((viz) => {
      if (active) setSvg(viz.renderString(String(children), { format: 'svg', engine: 'dot' }));
    }).catch((reason) => {
      if (active) setError(reason instanceof Error ? reason.message : String(reason));
    });
    return () => { active = false; };
  }, [children]);
  if (error) return <pre role="alert">Unable to render Graphviz diagram: {error}</pre>;
  return <div className={styles.diagram} role="img" aria-label={label} dangerouslySetInnerHTML={svg ? { __html: svg } : undefined}>
    {!svg && <span className={styles.loading}>Rendering diagram…</span>}
  </div>;
}
