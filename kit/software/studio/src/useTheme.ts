import { useEffect, useState } from 'react';
export function useTheme() {
  const [theme, setTheme] = useState(() => {
    try {
      const v = localStorage.getItem('nove-theme');
      return v === 'light' || v === 'dark' ? v : 'system';
    } catch {
      return 'system';
    }
  });
  useEffect(() => {
    const media = matchMedia('(prefers-color-scheme: dark)');
    const apply = () => {
      document.documentElement.dataset.theme =
        theme === 'system' ? (media.matches ? 'dark' : 'light') : theme;
    };
    apply();
    media.addEventListener('change', apply);
    try {
      localStorage.setItem('nove-theme', theme);
    } catch {
      /* Storage may be disabled; the theme still works for this session. */
    }
    return () => media.removeEventListener('change', apply);
  }, [theme]);
  return { theme, setTheme };
}
